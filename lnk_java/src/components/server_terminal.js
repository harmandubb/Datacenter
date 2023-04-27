import React from 'react';


const ServerTerminal = (props) => {
  return (
    <div className="container">
      <div className="black-box">
        <p className="text">{props.initial}</p>
      </div>
      <button className="send-button">Send</button>
    </div>
  );
};

export default ServerTerminal;