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

	"google.golang.org/api/idtoken"

	"github.com/mitchellh/mapstructure"

	"github.com/google/uuid"
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

type LoginResponseMessage struct {
	Verified bool   `json:"verified"`
	Email    string `json:"email"`
}

type ServerData struct {
	Host     string `json:"host"`
	Port     string `json:"port"`
	Username string `json:"Username"`
	Password string `json:"Password"`
}

type GeneralServerData struct {
	Name   string `json:"name"`
	Status string `json:"status"`
}

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

func createSessionToken() {
	sessionToken, err := uuid.NewUUID()
	if err != nil {
		fmt.Println("Error generating UUID:", err)
		return
	}

	fmt.Println(sessionToken.string())

}

func enableCORS(w *http.ResponseWriter) {
	(*w).Header().Set("Access-Control-Allow-Origin", "*")
	(*w).Header().Set("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE")
	(*w).Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
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
	loginResponseMessage.Email = email

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

func handleRequest(w http.ResponseWriter, r *http.Request) {
	enableCORS(&w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusNoContent)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
		return
	}

	//check if the request had been conpromised
	// checkCSRFToken(w, r)

	// fmt.Println("Content-Type:", r.Header.Get("Content-Type"))

	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Error reading request body", http.StatusInternalServerError)
		return
	}

	defer r.Body.Close()

	// fmt.Println("Request body:", string(body))

	var incomingMessage RequestMessage

	err = json.Unmarshal(body, &incomingMessage)

	// fmt.Println("JSON Message Struct:", incomingMessage)

	//Error starts below this

	if err != nil {
		fmt.Println("Error parsing JSON data:", err)
		http.Error(w, "Error parsing JSON data", http.StatusBadRequest)
		return
	}

	// fmt.Println("Received data:", incomingMessage.Credential)

	//Error ends here

	// if r.Method != http.MethodPost {
	// 	http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
	// 	return
	// }

	// Your normal request handling logic goes here
	w.Header().Set("Content-Type", "application/json")

	responseMessage := ResponseMessage{
		Status:  "Success",
		Message: "Does this work?",
	}

	jsonResponse, err := json.Marshal(responseMessage)

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

	// email_data := retriveEmailAssociatedServerInto(email)
	// json_data := getAllGeneralServerData(email_data)

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

		var data ServerData
		err := mapstructure.Decode(serverData, &data)
		if err != nil {
			log.Fatal(err)
		}

		fmt.Printf("Decoded server data: %+v\n", data.Host)

	}

}

func main() {
	fmt.Println("Set up server at 8080")

	createSessionToken()

	// http.HandleFunc("/", handleLoginRequest)

	// http.HandleFunc("/server", handleLoginRequest)

	// email_data := retriveEmailAssociatedServerInto("harmand1999@gmail.com")

	// getAllGeneralServerData(email_data)

	log.Fatal(http.ListenAndServe(":8080", nil))

}
