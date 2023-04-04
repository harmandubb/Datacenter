package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
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

func enableCORS(w *http.ResponseWriter) {
	(*w).Header().Set("Access-Control-Allow-Origin", "*")
	(*w).Header().Set("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE")
	(*w).Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
}

func handleRequest(w http.ResponseWriter, r *http.Request) {
	enableCORS(&w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusNoContent)
		return
	}

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

	if r.Method != http.MethodPost {
		http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
		return
	}

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

	http.HandleFunc("/", handleRequest)

	log.Fatal(http.ListenAndServe(":8080", nil))

}
