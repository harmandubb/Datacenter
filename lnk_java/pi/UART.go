package main

import (
	"fmt"
	"log"

	"github.com/tarm/serial"
)

func main() {
	// Make sure periph is initialized.
	// TODO: Use host.Init(). It is not used in this example to prevent circular
	// go package import.
	// Make sure periph is initialized.

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
	data := []byte("Hello, UART!\n")
	n, err := p.Write(data)
	if err != nil {
		log.Fatalf("Failed to write data to UART port: %v", err)
	}
	fmt.Printf("Wrote %d bytes to UART port.\n", n)

}
