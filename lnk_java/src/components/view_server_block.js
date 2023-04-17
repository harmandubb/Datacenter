import React from 'react';

function ServerBlock({ serverName, serverStatus }) {

    return (
        <div id="server_block">
          <p>Server Name: {serverName}</p>
          <p>Server Status: {serverStatus}</p>
          <button>Access Server</button>
        </div>
      );
}

export default ServerBlock;
