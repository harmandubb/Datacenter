package main

import (
	"fmt"
	"log"
	"os"
	"strings"

	//"bytes"
	"time"

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
	_, err = p.Write(data)
	_, err = p.Write([]byte("\r"))
	if err != nil {
		log.Fatalf("Failed to write data to UART port: %v", err)
	}
	//fmt.Printf("Wrote %d bytes to UART port.\n", n)

	buf := make([]byte, 8192)
	//var response []byte
	preVal := 0
	currentVal := 1
	time.Sleep(1 * time.Second)
	// Check if the buffer size is changing
	for preVal < currentVal {
		preVal = currentVal
		currentVal = len(buf)
		time.Sleep(100 * time.Millisecond)
	}

	//for {
	_, err = p.Read(buf)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf(string(buf[:len(buf)]) + "\n")

	//response = append(response, buf[:n]...)

	// Break the loop if a newline character is received.
	//if bytes.Contains(response, []byte("\n")) {
	//time.Sleep(1*time.Second)
	//fmt.Printf(string(response[:len(response)])+ "\n")
	//for _, letter := range response {
	//fmt.Println(string(letter))
	//}

	//break
	//}

}

func main() {
	fmt.Println("The GO Function is being executed")
	if len(os.Args) < 2 {
		//fmt.Println("Not enough arguments present")
		//os.Exit(1)
		transmitCommand("")
		//transmitCommand("\r")
	} else {
		command := strings.Join(os.Args[1:], " ")

		fmt.Println("echoing the command:", command)
		transmitCommand(command)
	}
}
