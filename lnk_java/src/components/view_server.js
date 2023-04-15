import React, { useState, useEffect } from 'react';
import { global } from 'styled-jsx/css';

const ServerInfo = () => {
  const [serverName, setServerName] = useState('');
  const [serverStatus, setServerStatus] = useState('');

  useEffect(() => {
    const fetchServerInfo = async () => {
      try {
        const response = await fetch('https://your-api-url.com/server-info');
        //Here I can send another request to the server given the infromation that I have gained 
        //TODO: Figure out how I can store the token where this component can acceses a unique identifier
        const data = await response.json();
        setServerName(data.serverName);
        setServerStatus(data.serverStatus);
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