package main

import (
	"bytes"
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/tarm/serial"
)

func transmitCommand(command string) {
	uartPortName := "/dev/ttyS1"

	// Configure UART communication settings.
	uartConfig := &serial.Config{
		Name:     uartPortName,
		Baud:     115200,
		StopBits: 1,
		Parity:   serial.ParityNone,
	}

	// Open the UART port with the specified configuration.
	p, err := serial.OpenPort(uartConfig)
	if err != nil {
		log.Fatalf("Failed to open UART port: %v", err)
	}
	defer p.Close()

	// Write data to the UART port.
	data := []byte(command)
	n, err := p.Write(data)
	if err != nil {
		log.Fatalf("Failed to write data to UART port: %v", err)
	}
	// fmt.Printf("Wrote %d bytes to UART port.\n", n)

	buf := make([]byte, 8192)
	var response []byte
	for {
		n, err = p.Read(buf)
		if err != nil {
			log.Fatal(err)
		}

		response = append(response, buf[:n]...)

		// Break the loop if a newline character is received.
		if bytes.Contains(response, []byte("\n")) {
			fmt.Println(string(response[:len(response)-1]))

			break
		}
	}

}

func main() {
	fmt.Println("The GO Function is being executed")
	if len(os.Args) < 2 {
		fmt.Println("Not enough arguments present")
		os.Exit(1)
	}

	command := strings.Join(os.Args[1:], " ")

	// fmt.Println("echoing the command:", command)
	transmitCommand(command)

}
