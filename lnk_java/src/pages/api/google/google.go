package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"reflect"
	"time"

	"google.golang.org/api/idtoken"

	"github.com/mitchellh/mapstructure"

	"github.com/google/uuid"

	"path/filepath"
)

// func validateCSRFToken(r *http.Request) error {
// 	csrfTokenCookie, err := r.Cookie("g_csrf_token")
// 	if err != nil {
// 		return errors.New("No CSRF token in Cookie")
// 	}

// 	csrfTokenBody := r.FormValue("g_csrf_token")
// 	if csrfTokenBody == "" {
// 		return errors.New("No CSRF token in post body")
// 	}

// 	if csrfTokenCookie.Value != csrfTokenBody {
// 		return errors.New("Failed to verify double submit cookie")
// 	}

// 	return nil
// }

type ResponseMessage struct {
	Status  string `json:"status"`
	Message string `json:"message"`
}

type RequestMessage struct {
	Credential string `json:"credential"`
}

type ServerRequestMessage struct {
	Token string `json:"token"`
}

type LoginResponseMessage struct {
	Verified bool      `json:"verified"`
	Email    string    `json:"email"`
	Token    string    `json:"token"`
	Expire   time.Time `json:"expire"`
}

type GeneralServerData struct {
	Name   string `json:"name"`
	Status string `json:"status"`
}

type UserInfo struct {
	Token   string                `json:"token"`
	Expire  time.Time             `json:"expire"`
	Servers map[string]ServerInfo `json:"servers"`
}

type ServerInfo struct {
	Host     string `json:"host"`
	Port     string `json:"port"`
	Username string `json:"username"`
	Password string `json:"password"`
}

const expireTimeOffset = 5

//----------TODO implement later-----------//
// func checkCSRFToken(w http.ResponseWriter, r *http.Request) {

// 	csrfTokenCookie, err := r.Cookie("g_csrf_token") //where is this cookie

// 	fmt.Println("Cookie Token:", csrfTokenCookie)
// 	fmt.Println("Error:", err)

// 	if err != nil {
// 		http.Error(w, "No CSRF token in Cookie.", http.StatusBadRequest)
// 		return
// 	}

// 	// csrfTokenBody := r.FormValue("g_csrf_token")
// 	// if csrfTokenBody == "" {
// 	// 	http.Error(w, "No CSRF token in post body.", http.StatusBadRequest)
// 	// 	return
// 	// }

// 	// if csrfTokenCookie.Value != csrfTokenBody {
// 	// 	http.Error(w, "Failed to verify double submit cookie.", http.StatusBadRequest)
// 	// 	return
// 	// }

// }

func retrivePublicGoogleKey() {
	url := "https://www.googleapis.com/oauth2/v3/certs"

	response, err := http.Get(url)
	if err != nil {
		log.Fatal(err)
	}

	defer response.Body.Close()

	//read the response body
	body, err := ioutil.ReadAll(response.Body)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println(string(body))
}

func verifyTokenSignature(idToken string) (bool, string) {
	clientID := "228928618151-a4lio74eijrst1fomvd46thcisdclqan.apps.googleusercontent.com"

	verifier, err := idtoken.NewValidator(context.Background())
	if err != nil {
		log.Fatal("Failed to creater verifier: %v", err)
		return false, ""
	}

	//Verify the ID token
	payload, err := verifier.Validate(context.Background(), idToken, clientID)
	if err != nil {
		log.Fatal("Failed to verify ID token: %v", err)
		return false, ""
	}

	//print the payload (claims) of the verified ID token
	fmt.Printf("Verfied ID token Payload: %+v\n", payload)
	fmt.Printf("\n Type of the payload: %v\n", reflect.TypeOf((payload.Claims["email"])))
	return true, payload.Claims["email"].(string)
}

func createSessionToken() (string, time.Time) {
	sessionToken, err := uuid.NewUUID()
	if err != nil {
		fmt.Println("Error generating UUID:", err)
		return "", time.Time{}
	}

	fmt.Println(sessionToken.String())

	expire := generateExpiryTime()

	return sessionToken.String(), expire

}

func generateExpiryTime() time.Time {
	currentDateAndTime := time.Now()
	newDateAndTime := currentDateAndTime.Add(expireTimeOffset * time.Minute)

	// Log the new date and time
	// fmt.Println("Current date and time:", currentDateAndTime)
	// fmt.Println("New date and time (5 minutes later):", newDateAndTime)
	// fmt.Println(reflect.TypeOf(currentDateAndTime))

	return newDateAndTime
}

func enableCORS(w *http.ResponseWriter) {
	(*w).Header().Set("Access-Control-Allow-Origin", "*")
	(*w).Header().Set("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE")
	(*w).Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
}

func updateSessionToken(fileName string, token string, expire time.Time) {
	file, err := os.Open(fileName)
	if err != nil {
		log.Fatal(err)
		return
	}

	defer file.Close()

	jsonData, err := ioutil.ReadAll(file)
	if err != nil {
		log.Fatal(err)
		return
	}

	// var userInfo UserInfo

	var userInfo map[string]interface{}

	err = json.Unmarshal(jsonData, &userInfo)
	if err != nil {
		log.Fatal(err)
		return
	}

	fmt.Println(userInfo["token"])
	fmt.Println(userInfo["expire"])

	userInfo["token"] = token
	userInfo["expire"] = expire

	modifiedJSON, err := json.MarshalIndent(userInfo, "", "  ")
	if err != nil {
		log.Fatal(err)
	}

	err = ioutil.WriteFile(fileName, modifiedJSON, 0644)
	if err != nil {
		log.Fatal(err)
	}

	// fmt.Println("JSON data modified successfully.")

}

func handleLoginRequest(w http.ResponseWriter, r *http.Request) {
	enableCORS(&w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusNoContent)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
		return
	}

	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Error reading request body", http.StatusInternalServerError)
		return
	}

	defer r.Body.Close()

	var loginMessage RequestMessage

	err = json.Unmarshal(body, &loginMessage)

	if err != nil {
		http.Error(w, "Error parsing JSON data", http.StatusBadRequest)
		return
	}

	var loginResponseMessage LoginResponseMessage

	verified, email := verifyTokenSignature(loginMessage.Credential)

	loginResponseMessage.Verified = verified

	if verified {
		//success case
		loginResponseMessage.Email = email
		token, expireTime := createSessionToken()
		loginResponseMessage.Token = token
		loginResponseMessage.Expire = expireTime

		//link the session token with the server json file
		updateSessionToken("./Users/"+email+".json", token, expireTime)
	} else {
		//fail case
		loginResponseMessage.Email = ""
		loginResponseMessage.Token = ""
		loginResponseMessage.Expire = time.Time{}

	}

	// Your normal request handling logic goes here
	w.Header().Set("Content-Type", "application/json")

	jsonResponse, err := json.Marshal(loginResponseMessage)

	if err != nil {
		http.Error(w, "Error generating JSON response", http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	w.Write(jsonResponse)

}

func retriveEmailAssociatedServerInto(email string) map[string]interface{} {
	//TODO: have a system where you can find what company is associated with an email and then determine the accesses that the user has
	file, err := os.Open("server_info.json")
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	//read the file content into a byte slice
	byteValue, err := ioutil.ReadAll(file)
	if err != nil {
		log.Fatal(err)
	}

	//unmarshel the JSON content into a map[string]interface()
	var total_data map[string]interface{}
	json.Unmarshal(byteValue, &total_data)

	email_data := total_data[email].(map[string]interface{})

	// fmt.Printf("This is the data associated with the user: %v\n", email_data)
	// fmt.Printf("This is the type of the data: %v\n", reflect.TypeOf(email_data))

	return email_data
}

func getAllGeneralServerData(email_data map[string]interface{}) []byte {
	//general serever data refers to data that somone is able to see on the general log in page
	//Example: server name and status

	servers_data := []GeneralServerData{}

	for key, servers := range email_data {
		serverData := servers.(map[string]interface{})
		fmt.Printf("Server %v data %v\n", key, serverData)

		fmt.Printf("Type of Key: %v\n", reflect.TypeOf(key))

		generalServerData := GeneralServerData{
			Name:   key,
			Status: "OK",
		}

		servers_data = append(servers_data, generalServerData)

	}

	json_servers_data, err := json.Marshal(servers_data)

	if err != nil {
		log.Fatal(err)
	}

	return json_servers_data

}

func handleServerRequest(w http.ResponseWriter, r *http.Request) {
	enableCORS(&w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusNoContent)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
		return
	}

	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Error reading request body", http.StatusInternalServerError)
		return
	}

	defer r.Body.Close()

	var serverRequestMessage ServerRequestMessage

	err = json.Unmarshal(body, &serverRequestMessage)

	if err != nil {
		http.Error(w, "Error parsing JSON data", http.StatusBadRequest)
		return
	}

	serverPath := findTokenUser(serverRequestMessage.Token)

	//Now extract the infromation needed from the json file to be sent out:

	jsonData, err := ioutil.ReadFile(serverPath)
	if err != nil {
		fmt.Printf("Error reading file %s: %v\n", serverPath, err)
		return
	}

	var userInfo UserInfo

	err = json.Unmarshal(jsonData, &userInfo)
	if err != nil {
		log.Fatal(err)
		return
	}

	for serverName, server := range userInfo.Servers {
		fmt.Printf("\nServer: %s\n", serverName)
		fmt.Println("Host:", server.Host)
		fmt.Println("User:", server.Username)
		fmt.Println("Port:", server.Port)
		fmt.Println("Pass:", server.Password)

	}

	//return a json file with the server infromation given
}

func findTokenUser(token string) string {
	dir := "./Users" // Replace with the path to your folder

	files, err := ioutil.ReadDir(dir)
	if err != nil {
		fmt.Printf("Error reading the directory: %v\n", err)
		return ""
	}

	for _, file := range files {
		if !file.IsDir() {
			filePath := filepath.Join(dir, file.Name())

			jsonData, err := ioutil.ReadFile(filePath)
			if err != nil {
				fmt.Printf("Error reading file %s: %v\n", filePath, err)
				return ""
			}

			var userInfo map[string]interface{}

			err = json.Unmarshal(jsonData, &userInfo)
			if err != nil {
				log.Fatal(err)
				return ""
			}

			fmt.Println(userInfo["token"])

			if userInfo["token"] == token {
				return filePath
			}

		}
	}

	return ""
}

func retriveUserServerInfo(email string) {
	//TODO: have a system where you can find what company is associated with an email and then determine the accesses that the user has
	file, err := os.Open("server_info.json")
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	//read the file content into a byte slice
	byteValue, err := ioutil.ReadAll(file)
	if err != nil {
		log.Fatal(err)
	}

	//unmarshel the JSON content into a map[string]interface()
	var email_data map[string]interface{}
	json.Unmarshal(byteValue, &email_data)

	fmt.Printf("This is the data associated with the user: %v\n", email_data)
	fmt.Printf("This is the type of the data: %v\n", reflect.TypeOf(email_data))

	//access the nested JSOn object
	servers_data := email_data[email].(map[string]interface{})
	for key, servers := range servers_data {
		serverData := servers.(map[string]interface{})
		fmt.Printf("Server %v data %v\n", key, serverData)

		var data ServerInfo
		err := mapstructure.Decode(serverData, &data)
		if err != nil {
			log.Fatal(err)
		}

		fmt.Printf("Decoded server data: %+v\n", data.Host)

	}

}

func main() {
	fmt.Println("Set up server at 8080")

	http.HandleFunc("/", handleLoginRequest)

	http.HandleFunc("/server", handleServerRequest)

	// fmt.Println(findTokenUser("06e207fa-db59-11ed-9ddf-0c4de9cb1e33"))

	// email_data := retriveEmailAssociatedServerInto("harmand1999@gmail.com")

	// getAllGeneralServerData(email_data)

	log.Fatal(http.ListenAndServe(":8080", nil))

}
