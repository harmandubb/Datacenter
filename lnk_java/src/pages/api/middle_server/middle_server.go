package main

import (
	"bytes"
	"fmt"
	"log"
	"time"

	"golang.org/x/crypto/ssh"
)

func main() {
<<<<<<< HEAD
	host := "192.168.1.175" //TODO: pull credentails from a data base server to pupolaute based on the server that one wants to acceses.
=======
	host := "192.168.1.175"
>>>>>>> 675da4dbb7ac15ba19e6feef20195cc49b89daa9
	port := 22
	username := "pi"
	password := "4715"

	// pKey := []byte("<privatekey>")

	// var signer ssh.Signer
	// var err error

	// signer, err = ssh.ParsePrivateKey(pKey)

	// if err != nil {
	// 	fmt.Println(err.Error())
	// }

	// Configure SSH client
	config := &ssh.ClientConfig{
		User: username,
		Auth: []ssh.AuthMethod{
			ssh.Password(password),
			// ssh.PublicKeys(signer),
		},
<<<<<<< HEAD
		HostKeyCallback: ssh.InsecureIgnoreHostKey(), //TODO:create security around this to make sure that middle man attack cannot occur.
=======
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
>>>>>>> 675da4dbb7ac15ba19e6feef20195cc49b89daa9
		Timeout:         10 * time.Second,
	}

	// Connect to the remote server
	address := fmt.Sprintf("%s:%d", host, port)
	fmt.Println(address)
	client, err := ssh.Dial("tcp", address, config)
	if err != nil {
		log.Fatalf("Failed to connect: %s", err)
	}
	defer client.Close()

	fmt.Println("I am here")

	// Run a command on the remote server
	cmd := "uname -a"
	output, err := runCommand(client, cmd)
	if err != nil {
		log.Fatalf("Failed to run command: %s", err)
	}

	fmt.Println("Output:")
	fmt.Println(output)
}

func runCommand(client *ssh.Client, cmd string) (string, error) {
	session, err := client.NewSession()
	if err != nil {
		return "", err
	}
	defer session.Close()

	var stdoutBuf bytes.Buffer
	session.Stdout = &stdoutBuf

	err = session.Run(cmd)
	if err != nil {
		return "", err
	}

	return stdoutBuf.String(), nil
}
