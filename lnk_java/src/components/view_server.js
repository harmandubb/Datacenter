import React, { useState, useEffect } from 'react';
import { global } from 'styled-jsx/css';
import ServerBlock from './view_server_block';


const url = "http://localhost:8080/server"

const ServerInfo = () => {
  // const [serverName, setServerName] = useState('');
  // const [serverStatus, setServerStatus] = useState('');
  const [serverData, setServerData] = useState([]);

  useEffect(() => {
    const fetchServerInfo = async () => {
      console.log("REQUEST MADE")
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

              // for (let i = 0; i < data.length; i++) {
              //   setServerName(data[i].Name);
              //   setServerStatus(data[i].Status);
              // }

              setServerData(data);
              console.log("Type of server data:",typeof(serverData));
            }) 

      } catch (error) {
        console.error('Error fetching server info:', error);
      }
    };

    fetchServerInfo();
  }, []);

  const blocks = serverData.map((value, index) => (
    <ServerBlock serverName={value.Name} serverStatus={value.Status} />
  ));

  return (
    <div id="servers">
      <div>{blocks}</div>
    </div>
  );
};

export default ServerInfo;