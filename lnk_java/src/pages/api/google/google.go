package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"reflect"
	"strings"
	"time"

	"google.golang.org/api/idtoken"

	"github.com/mitchellh/mapstructure"

	"github.com/google/uuid"

	"path/filepath"

	"golang.org/x/crypto/ssh"
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

type UserInfo struct {
	Token   string                `json:"token"`
	Expire  time.Time             `json:"expire"`
	Servers map[string]ServerInfo `json:"servers"`
}

type ServerInfo struct {
	Name     string `json:"name"`
	Host     string `json:"host"`
	Port     string `json:"port"`
	Username string `json:"username"`
	Password string `json:"password"`
}

type GeneralServerInfo struct {
	Status string `json:"status` //TODO: Implement code that checks the status of the server
	Name   string `json:"status`
}

type GeneralInfoAllServers map[string]GeneralServerInfo

type AccessServerInfo struct {
	Name  string `json:"name`
	Token string `json:"token"` //TODO: Implement the expire data mechansims in this struct after
}

type ServerResponse struct {
	Name     string `json:"name`
	Token    string `json:"token"`
	Response string `json:"response"`
}

const expireTimeOffset = 5

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
	// fmt.Printf("Verfied ID token Payload: %+v\n", payload)
	// fmt.Printf("\n Type of the payload: %v\n", reflect.TypeOf((payload.Claims["email"])))
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

func handleServerRequest(w http.ResponseWriter, r *http.Request) {
	enableCORS(&w)

	fmt.Println("Inthe Server Request Funciton")

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

	//TODO: Check if the token expirey is still valid on the server end. If not then force the website to load again

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

	var generalServerInfoAll []GeneralServerInfo
	// var generalServerInfo GeneralServerInfo

	for serverName := range userInfo.Servers {
		var generalServerInfo GeneralServerInfo

		generalServerInfo.Name = serverName
		generalServerInfo.Status = "OK"

		generalServerInfoAll = append(generalServerInfoAll, generalServerInfo)

		// fmt.Printf("\nServer: %s\n", serverName)
		// fmt.Println("Host:", server.Host)
		// fmt.Println("User:", server.Username)
		// fmt.Println("Port:", server.Port)
		// fmt.Println("Pass:", server.Password)
	}

	//return a json file with the server infromation given
	// fmt.Println("Json data:", generalServerInfoAll) //Might have to change the structure if the JSON data labels are not put in

	generalServerInfoResponse, err := json.Marshal(generalServerInfoAll)
	if err != nil {
		log.Fatal(err)
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write(generalServerInfoResponse)

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

			// fmt.Println(userInfo["token"])

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

func establishConnection(h string, p string, user string, pass string) (*ssh.Session, error) {
	host := h //TODO: pull credentails from a data base server to pupolaute based on the server that one wants to acceses.
	port := p
	username := user
	password := pass

	// Configure SSH client
	config := &ssh.ClientConfig{
		User: username,
		Auth: []ssh.AuthMethod{
			ssh.Password(password),
			// ssh.PublicKeys(signer),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(), //TODO:create security around this to make sure that middle man attack cannot occur.
		Timeout:         10 * time.Second,
	}

	// Connect to the remote server
	address := fmt.Sprintf("%s:%s", host, port)
	fmt.Println(address)
	client, err := ssh.Dial("tcp", address, config)
	if err != nil {
		log.Fatalf("Failed to connect: %s", err)
		return nil, err
	}

	session, err := establishSession(client)

	// defer client.Close()

	return session, nil
}

func establishSession(client *ssh.Client) (*ssh.Session, error) {
	session, err := client.NewSession()
	if err != nil {
		log.Fatal(err)
	}

	return session, nil
}

func runCommand(session *ssh.Session, cmd []string) (string, error) {
	var stdoutBuf bytes.Buffer
	session.Stdout = &stdoutBuf

	commands := strings.Join(cmd, "; ")
	initial_cmd := "uname -a"

	fmt.Println(commands)

	fmt.Println("ERROR CHECK")

	err := session.Run(initial_cmd)
	if err != nil {
		return "", err
	}

	return stdoutBuf.String(), nil
}

func handleAccessRequest(w http.ResponseWriter, r *http.Request) {
	enableCORS(&w)

	fmt.Println("Inthe Access Request Funciton")

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

	var accessServerInfo AccessServerInfo

	err = json.Unmarshal(body, &accessServerInfo)
	if err != nil {
		log.Fatal(err)
	}

	serverPath := findTokenUser(accessServerInfo.Token)

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

	var serverInfo ServerInfo

	serverInfo.Host = userInfo.Servers[accessServerInfo.Name].Host
	serverInfo.Password = userInfo.Servers[accessServerInfo.Name].Password
	serverInfo.Port = userInfo.Servers[accessServerInfo.Name].Port
	serverInfo.Username = userInfo.Servers[accessServerInfo.Name].Username

	session, err := establishConnection(serverInfo.Host, serverInfo.Port, serverInfo.Username, serverInfo.Password)

	cmd := []string{
		"uname -a",
	}

	output, err := runCommand(session, cmd)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Println("OUTPUT:", output)

	if err != nil {
		//TODO: Do something on the front end page to show the error
	} else {
		//return a baseline message
		var serverResp ServerResponse

		serverResp.Name = accessServerInfo.Name
		serverResp.Token = accessServerInfo.Token
		serverResp.Response = output

		respJson, err := json.Marshal(serverResp)
		if err != nil {
			//TODO: send the error response for the server
			fmt.Println("error:", err)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		w.Write(respJson)

		// return session

	}
}

func main() {
	fmt.Println("Set up server at 8080")

	http.HandleFunc("/", handleLoginRequest)

	http.HandleFunc("/server", handleServerRequest) //TODO: Only recreate the token once and see if the expire time dictates that the token needs to be created again.

	http.HandleFunc("/access", handleAccessRequest)

	// http.HandleFunc("/cmd", handleCMDRequest)

	log.Fatal(http.ListenAndServe(":8080", nil))

}
