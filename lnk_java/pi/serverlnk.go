package main

import (
	"fmt"
	"os"
)

func passCommands(input string) {
	fmt.Println("echoing the string:", input)
}

func main() {
	fmt.Println("The GO Function is being executed")
	if len(os.Args) < 1 {
		fmt.Println("Not enough arguments present")
		os.Exit(1)
	}

	command := os.Args[1]
	passCommands(command)

}
