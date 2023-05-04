package main

import (
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
	n, err = p.Read(buf)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Println(string(buf[:n]))

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
