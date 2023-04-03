import React from 'react';

import { GoogleLogin } from '@react-oauth/google';

async function googleCredentialsCheck(url = "http://localhost:8080", credentials) {
    const options = {
        method: 'POST',
        headers: {
            "Content-Type": "application/json",
            // 'Access-Control-Allow-Origin': "*",
            // 'Access-Control-Allow-Methods': ('POST', 'OPTIONS', 'HEAD', 'GET', 'PUT', 'DELETE'),
            // 'Access-Control-Allow-Headers': ('Content-Type', 'Authorization'),
        },
        body: JSON.stringify(credentials),
    }

    await fetch(url,options)
        .then((response) => {
            console.log("Fetch was sucessful:", response);
            console.log("URL:", response.url);
            console.log("Body:", response.body);

            if (response.status == 200){
                console.log("Success");
            } else {
                console.log("Fail");
            }
        })
        .catch((err) => {
            console.log("An Error has occured"),
            console.log("fetch returned an error:", err) }
            );
            

  }
  

const google = () => {

    return (
        <GoogleLogin
            onSuccess={credentialResponse => {
              console.log(credentialResponse);
              googleCredentialsCheck("http://localhost:8080", credentialResponse);



            }}
            onError={() => {
              console.log('Login Failed');
            }}
          />
    )
}

export default google;
  