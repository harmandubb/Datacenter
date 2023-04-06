package main

import (
	"bytes"
	"fmt"
	"log"
	"time"

	"golang.org/x/crypto/ssh"
)

func main() {
	host := "your_remote_server_address"
	port := 22
	username := "your_username"
	password := "your_password"
	pKey := []byte("<privatekey>")

	var signer ssh.Signer
	var err error

	signer, err = ssh.ParsePrivateKey(pKey)

	if err != nil {
		fmt.Println(err.Error())
	}

	// Configure SSH client
	config := &ssh.ClientConfig{
		User: username,
		Auth: []ssh.AuthMethod{
			ssh.PublicKeys(signer),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
		Timeout:         10 * time.Second,
	}

	// Connect to the remote server
	address := fmt.Sprintf("%s:%d", host, port)
	client, err := ssh.Dial("tcp", address, config)
	if err != nil {
		log.Fatalf("Failed to connect: %s", err)
	}
	defer client.Close()

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
