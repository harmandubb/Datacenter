package main

import (
	"bytes"
	"fmt"
	"log"
	"time"

	"golang.org/x/crypto/ssh"
)

func main() {
	host := "192.168.1.109" //TODO: pull credentails from a data base server to pupolaute based on the server that one wants to acceses.
	port := 22
	username := "root"
	password := "odroid"

	fmt.Println("Host:", host)
	fmt.Println("Port:", port)
	fmt.Println("Username:", username)
	fmt.Println("Password:", password)

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
		HostKeyCallback: ssh.InsecureIgnoreHostKey(), //TODO:create security around this to make sure that middle man attack cannot occur.
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

	fmt.Println("I am here", client)

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
