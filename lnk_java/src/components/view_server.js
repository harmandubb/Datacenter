import React, { useState, useEffect } from 'react';
import { global } from 'styled-jsx/css';

const url = "http://localhost:8080/server"

const ServerInfo = () => {
  const [serverName, setServerName] = useState('');
  const [serverStatus, setServerStatus] = useState('');

  useEffect(() => {
    const fetchServerInfo = async () => {
      try {
        let sessionStorageJSON = JSON.parse(sessionStorage.getItem("serverLNKSession"));
        console.log("Sessionstorage JSON format:", sessionStorageJSON);
        console.log("Token:", sessionStorageJSON.token);

        let data = {
          token: sessionStorageJSON.token
        };
        const options = {
            method: 'POST',
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(data),
        }

      console.log("Request JSON options:", options);

       await fetch(url,options)
            .then((response) => {
              console.log("Raw response data:", response);
              return response.json();
            }).then((data) => {
              console.log("Parsed data",data);
              console.log(data);
            }) 

      } catch (error) {
        console.error('Error fetching server info:', error);
      }
    };

    fetchServerInfo();
  }, []);

  return (
    <div id="server_block">
      <p>Server Name: {serverName}</p>
      <p>Server Status: {serverStatus}</p>
      <button>Access Server</button>
    </div>
  );
};

export default ServerInfo;