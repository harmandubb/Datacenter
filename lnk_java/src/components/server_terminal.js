import React from 'react';
import { useState } from 'react';
import TerminalLines from './terminal_lines';

function ServerTerminal(props) {
  const [command, setCommand] = useState('');
  const [messages, setMessages] = useState([]);


  const handleCommandChange = (event) => {
    setCommand(event.target.value);
  };

  const addMessage = (currentMessage) => {
    setMessages(prevMessages => [...prevMessages, currentMessage]);
  };

  async function handleSubmit(){
    // Make a post to the back end for the server update
    let url = "http://localhost:8080/cmd";
  
    let sessionStorageJSON = JSON.parse(sessionStorage.getItem("serverLNKSession"));
    // let serverTerminalStorageJSON = JSON.parse(sessionStorage.getItem("serverTerminalSession"));

    let cmd = "serverlnk " + command 

    let data = {
      Name: props.name,
      Token: sessionStorageJSON.token,
      CMD: cmd,
    };

    console.log("Command in the struct:",data.CMD);

    // setMessages([messages, "User: " + command])

    console.log("messages structure:")
  
    
    const options = {
      method: 'POST',
      headers: {
          "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    }
  
    // console.log("Options:",options)
    await fetch(url,options)
    .then((response) => {
      return response.json();
      
    }).then((data) =>{
      let currentMessage = ["User: " + command, "Server: " + data.response]

      console.log("Current Message:", currentMessage);
      addMessage(currentMessage);
      console.log("printing updated Message:",messages);
    })
    console.log(command);
    setCommand('');
  };

  const terminalLines = messages.map((data, index) => (
    <TerminalLines key={index} user={data[0]} server={data[1]} />
  ));

  return (
    <div className="container">
      <div className="black-box">
        <p className="text">{props.initial}</p>
        {/* Communication block can be present here to make the terminal read out prompts and give answers as they are given */}
        <div>
          {terminalLines}
        </div>
      </div>
      <div className="input-container">
        <input
          type="text"
          name="userRequest"
          className="command-input"
          value={command}
          placeholder="Enter command"
          onChange={handleCommandChange}
        />
        <button className="send-button" onClick={handleSubmit}>
          Send
        </button>
      </div>
    </div>
  );
};

export default ServerTerminal;

function handleSubmit(){
  setInputText()
}