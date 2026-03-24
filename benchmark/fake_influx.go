package main

import (
	"fmt"
	// "io"
	"log"
	"net/http"
)

func metricsHandler(w http.ResponseWriter, r *http.Request) {
	// // Read the body content
	// body, err := io.ReadAll(r.Body)
	// if err != nil {
	// 	log.Printf("Error reading body: %v", err)
	// 	http.Error(w, "Can't read body", http.StatusBadRequest)
	// 	return
	// }
	// defer r.Body.Close()
	//
	// // Print details to the console
	// fmt.Printf("--- Incoming Request ---\n")
	// fmt.Printf("Method: %s\n", r.Method)
	// fmt.Printf("Path:   %s\n", r.URL.Path)
	//
	// if len(body) > 0 {
	// 	fmt.Printf("Payload: %s\n", string(body))
	// } else {
	// 	fmt.Println("Payload: [Empty]")
	// }
	// fmt.Printf("------------------------\n\n")

	// io-fs elasticsearch.c expectation
	if r.Method == http.MethodPost {
		w.WriteHeader(http.StatusCreated)
		w.Write([]byte("Created"))
	} else {
		w.WriteHeader(http.StatusOK)
	}
}

func main() {
	port := 8086
	mux := http.NewServeMux()

	mux.HandleFunc("/", metricsHandler)

	fmt.Printf("Logging metrics Go server listening on port %d...\n", port)

	server := &http.Server{
		Addr:    fmt.Sprintf(":%d", port),
		Handler: mux,
	}

	if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		log.Fatalf("Server failed to start: %v\n", err)
	}
}
