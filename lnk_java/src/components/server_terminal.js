import React from 'react';
import { useState } from 'react';
import TerminalLines from './terminal_lines';

function ServerTerminal(props) {
  const [command, setCommand] = useState('');
  const [messages, setMessages] = useState([]);


  const handleCommandChange = (event) => {
    setCommand(event.target.value);
  };

  async function handleSubmit(){
    // Make a post to the back end for the server update
    let url = "http://localhost:8080/cmd";
  
    let sessionStorageJSON = JSON.parse(sessionStorage.getItem("serverLNKSession"));
    // let serverTerminalStorageJSON = JSON.parse(sessionStorage.getItem("serverTerminalSession"));

    let data = {
      Name: props.name,
      Token: sessionStorageJSON.token,
      CMD: command,
    };

    setMessages([messages, "User: " + command])
  
    
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
      console.log("I am here");
      //TODO: check if the credentials still checkout at this point
      setMessages((prevMessages) => [prevMessages,"Server: " + data.response])
      console.log(messages);
    })
    console.log(command);
    setCommand('');
  };

  const terminalLines = messages.map((data, index) => (
    <TerminalLines key={index} message={data} />
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