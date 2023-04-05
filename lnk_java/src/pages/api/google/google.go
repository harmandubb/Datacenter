package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"

	"google.golang.org/api/idtoken"
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
	Verified bool `json:"verified"`
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

func verifyTokenSignature(idToken string) bool {
	clientID := "228928618151-a4lio74eijrst1fomvd46thcisdclqan.apps.googleusercontent.com"

	verifier, err := idtoken.NewValidator(context.Background())
	if err != nil {
		log.Fatal("Failed to creater verifier: %v", err)
		return false
	}

	//Verify the ID token
	payload, err := verifier.Validate(context.Background(), idToken, clientID)
	if err != nil {
		log.Fatal("Failed to verify ID token: %v", err)
		return false
	}

	//print the payload (claims) of the verified ID token
	fmt.Printf("Verfied ID token Payload: %+v\n", payload)
	return true

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

	verified := verifyTokenSignature(loginMessage.Credential)

	loginResponseMessage.Verified = verified

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

func main() {
	fmt.Println("Set up server at 8080")

	http.HandleFunc("/", handleLoginRequest)

	log.Fatal(http.ListenAndServe(":8080", nil))

}
