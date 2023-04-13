import React from 'react';

import { GoogleLogin } from '@react-oauth/google';
import { useRouter } from 'next/router';


async function googleCredentialsCheck(router ,url = "http://localhost:8080", credentials) {

    // console.log("Credentials:", credentials.credential);
    // console.log("cleint ID:", credentials.clientId);
    // console.log("Select:", credentials.select_by);

    let data = {
        credential: credentials.credential
    };


    const options = {
        method: 'POST',
        headers: {
            "Content-Type": "application/json",
            // 'Access-Control-Allow-Origin': "*",
            // 'Access-Control-Allow-Methods': ('POST', 'OPTIONS', 'HEAD', 'GET', 'PUT', 'DELETE'),
            // 'Access-Control-Allow-Headers': ('Content-Type', 'Authorization'),
        },
        body: JSON.stringify(data),
    }

     // Log the JSON data before sending it
    console.log("Sending JSON data:", data);


    // console.log("Options:",options)
    await fetch(url,options)
        .then((response) => {
            console.log("Fetch was sucessful:", response)
            return response.json();
        })
        .then((data) => {
            console.log("Server response:", data)
            if(data.verified){
                //redirect to the server page here
                router.push('./server');
            } else {
                // Produce Error message on page
            }
        })
        .catch((err) => {
            console.log("An Error has occured"),
            console.log("fetch returned an error:", err) }
            );
            

  }
  

const google = () => {
   const router = useRouter();

    return (
        <GoogleLogin
            onSuccess={credentialResponse => {
                console.log("Below is the credential response");
                console.log(credentialResponse); 
                googleCredentialsCheck(router,"http://localhost:8080", credentialResponse);
            }}
            onError={() => {
              console.log('Login Failed');
            }}
          />
    )
}

export default google;
  