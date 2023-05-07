package main

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"strings"
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
	fmt.Printf("Wrote %d bytes to UART port.\n", n)

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
	fmt.Printf("The buffer holds:", string(buf[:len(buf)])+"\n")
	fileName := "/home/read.txt"
	buf = bytes.Trim(buf[:len(buf)], "\x00")
	buf = bytes.TrimLeft(buf[:len(buf)], "\x0a")
	buf = bytes.TrimLeft(buf[:len(buf)], "\x0d")
	buf = bytes.TrimLeft(buf[:len(buf)], "\x0a")
	buf = append(buf, byte('\n'))
	fmt.Println(buf[:len(buf)])
	content := string(buf[:len(buf)])
	file, err := os.Create(fileName)
	if err != nil {
		log.Fatal(err)
	}

	err = ioutil.WriteFile(fileName, []byte(content), 0644)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("Successfully wrote to file: %s \nThe content: %s\n", fileName, content)

	err = file.Close()
	if err != nil {
		log.Fatal(err)
	}
}

func main() {
	//app := "echo"
	//arg := "the GO Function is being executied"
	//cmd := exec.Command(app, arg)
	//output, _ := cmd.Output()
	//fmt.Println(string(output))
	//os.Stdout.Write([]byte("The GO Function is being executed"))
	if len(os.Args) < 2 {
		//fmt.Println("Not enough arguments present")
		//os.Exit(1)
		transmitCommand("")
		//transmitCommand("\r")
	} else {
		command := strings.Join(os.Args[1:], " ")

		fmt.Println("echoing the input command:", command)
		transmitCommand(command)
	}
	//attempt to read commands from the file
	app := "bash"
	arg0 := "-c"
	arg1 := "while read line; do echo $line; done < read.txt"
	cmd := exec.Command(app, arg0, arg1)
	output, _ := cmd.Output()
	//fmt.Println("Size of the read file:", len(output))
	fmt.Println(string(output))

}
