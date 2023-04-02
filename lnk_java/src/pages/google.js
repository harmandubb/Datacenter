import React from 'react';

import { GoogleLogin } from '@react-oauth/google';

async function googleCredentialsCheck(url = "http://localhost:8080", credentials) {
    const options = {
        method: 'POST',
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify(credentials),
    }

    await fetch(url,options)
        .then((response) => {
            console.log("Fetch was sucessful:", response);
            if (response.status == 200){
                console.log("Success");
            } else {
                console.log("Fail");
            }
        })
        .catch((err) =>
            console.log("fetch returned an error:", err));

  }
  

const google = () => {

    return (
        <GoogleLogin
            onSuccess={credentialResponse => {
              console.log(credentialResponse);
              googleCredentialsCheck(credentialResponse);



            }}
            onError={() => {
              console.log('Login Failed');
            }}
          />
    )
}

export default google;
  