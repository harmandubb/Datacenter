import React from 'react';


const ServerTerminal = (props) => {
  return (
    <div className="container">
      <div className="black-box">
        <p className="text">{props.initial}</p>
      </div>
      <div className="input-container">
        <input
          type="text"
          className="command-input"
        //   value={command}
        //   onChange={handleCommandChange}
          placeholder="Enter command"
        />
        <button className="send-button"> {/* onClick={handleSubmit}> */}
          Send
        </button>
      </div>
    </div>
  );
};

export default ServerTerminal;