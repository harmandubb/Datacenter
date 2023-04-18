import React from 'react';




function ServerBlock({ serverName, serverStatus }) {
    return (
        <div id={serverName}>
          <p>Server Name: {serverName}</p>
          <p>Server Status: {serverStatus}</p>
          <button onClick={() => handleClick(serverName)}>Access Server</button>
        </div>
      );
}


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
      return response.json();
  })
  .then((data) => {
      
  })
  .catch((err) => {
      console.log("An Error has occured"),
      console.log("fetch returned an error:", err) }
      );


}

export default ServerBlock;
