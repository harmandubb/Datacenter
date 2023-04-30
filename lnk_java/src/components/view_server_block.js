import React from 'react';
import ServerTerminal from './server_terminal';
import { useState } from 'react';

function ServerBlock({ serverName, serverStatus }) {
  const [isVisible, setIsVisible] = useState(false);

  const toggleTerminal = () => {
    setIsVisible(!isVisible);
  };

  const [initializedServer, setInitializedServer] = useState("");

  const initializedServerNow = (server_string) => {
    setInitializedServer(server_string);
  };

  const handleClick = async (serverName) => {
    let url = "http://localhost:8080/access";
  
    let sessionStorageJSON = JSON.parse(sessionStorage.getItem("serverLNKSession"));
  
    let data = {
      Name: serverName,
      Token: sessionStorageJSON.token
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
    .then((response) => {
      
      console.log("After server is initialized:", response);
      const serverTerminalSessionStorageObject = {
        name: serverName
      }

      sessionStorage.setItem("serverTerminalSession", JSON.stringify(serverTerminalSessionStorageObject));

      return response.json();
    })
    .then((data) => {
      initializedServerNow(data.response);
      toggleTerminal();
    })
    .catch((err) => {
        console.log("An Error has occured");
        console.log("fetch returned an error:", err); }
        );
  
  
  }

    return (
        <div id={serverName}>
          <p>Server Name: {serverName}</p>
          <p>Server Status: {serverStatus}</p>
          <button onClick={() => handleClick(serverName)}>Access Server</button>
          {isVisible && (
            <ServerTerminal initial={initializedServer} name={serverName}/>
          )}
        </div>
      );
}





export default ServerBlock;
