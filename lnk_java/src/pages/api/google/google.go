package main

import (
	"encoding/json"
	"fmt"
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

type Message struct {
	Status  string `json:"status"`
	Message string `json:"message"`
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

	if r.Method != http.MethodPost {
		http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
		return
	}

	// Your normal request handling logic goes here
	w.Header().Set("Content-Type", "application/json")

	responseMessage := Message{
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

	// fmt.Fprint(w, "Hello, this is a response with CORS enabled!")
}

func main() {
	fmt.Println("Set up server at 8080")
	// http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
	// 	err := validateCSRFToken(r)
	// 	if err != nil {
	// 		http.Error(w, err.Error(), http.StatusBadRequest)
	// 		return
	// 	}

	// 	// Handle the request if CSRF token is valid
	// 	fmt.Fprint(w, "CSRF token is valid")
	// })

	// http.ListenAndServe(":8080", nil)

	http.HandleFunc("/", handleRequest)

	log.Fatal(http.ListenAndServe(":8080", nil))

}
