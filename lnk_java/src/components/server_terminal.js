import React from 'react';
import { useState } from 'react';

function ServerTerminal(props) {
  const [command, setCommand] = useState('');

  const handleCommandChange = (event) => {
    setCommand(event.target.value);
  };

  async function handleSubmit(){
    // Make a post to the back end for the server update
    let url = "http://localhost:8080/cmd";
  
    let sessionStorageJSON = JSON.parse(sessionStorage.getItem("serverLNKSession"));
  
    let data = {
      Name: props.name,
      Token: sessionStorageJSON.token,
      CMD: command,
    };
  
    
    const options = {
      method: 'POST',
      headers: {
          "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    }
  
    // console.log("Options:",options)
    await fetch(url,options)
    console.log(command);
    setCommand('');
  };

  return (
    <div className="container">
      <div className="black-box">
        <p className="text">{props.initial}</p>
        {/* Communication block can be present here to make the terminal read out prompts and give answers as they are given */}
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