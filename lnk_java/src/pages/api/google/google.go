package main

import (
	"errors"
	"fmt"
	"net/http"
)

func validateCSRFToken(r *http.Request) error {
	csrfTokenCookie, err := r.Cookie("g_csrf_token")
	if err != nil {
		return errors.New("No CSRF token in Cookie")
	}

	csrfTokenBody := r.FormValue("g_csrf_token")
	if csrfTokenBody == "" {
		return errors.New("No CSRF token in post body")
	}

	if csrfTokenCookie.Value != csrfTokenBody {
		return errors.New("Failed to verify double submit cookie")
	}

	return nil
}

func main() {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		err := validateCSRFToken(r)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}

		// Handle the request if CSRF token is valid
		fmt.Fprint(w, "CSRF token is valid")
	})

	http.ListenAndServe(":8080", nil)
}
