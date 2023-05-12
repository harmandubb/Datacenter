import React from 'react';
import ServerTerminal from './server_terminal';
import { useState } from 'react';

function TerminalLines({user,server}) {
  

    return (
        <div>
            <div>
                {user}
            </div>
            <div>
                {server}
            </div>
        </div>
      );
}


export default TerminalLines;
