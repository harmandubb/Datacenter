import React from 'react';
import ServerTerminal from './server_terminal';
import { useState } from 'react';

function TerminalLines({message}) {
  

    return (
        <p>
            {message}
        </p>
      );
}


export default TerminalLines;
