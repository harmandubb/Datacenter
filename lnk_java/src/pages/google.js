import React from 'react';

import { GoogleLogin } from '@react-oauth/google';
import { useRouter } from 'next/router';

import { CookiesProvider } from 'react-cookie';
import { useCookies } from 'react-cookie';


function setUserSessionToken(sessionToken, expireTime){
    // const currentDateAndTime = new Date();
    // const expires = new Date(currentDateAndTime.getTime() + 5 * 60 * 1000);

    let sessionObject = {
        expiresAt: expireTime,
        token: sessionToken,
    }

    sessionStorage.setItem("serverLNKSession", JSON.stringify(sessionObject));
    // sessionStorage.setItem("serverLNKSessionToken", "sessionToken");
    // sessionStorage.setItem("serverLNKSessionExpirey", "expireTime");

}




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
        },
        body: JSON.stringify(data),
    }

     // Log the JSON data before sending it
    console.log("Sending JSON data:", data);


    // console.log("Options:",options)
    await fetch(url,options)
        .then((response) => {
            return response.json();
        })
        .then((data) => {
            console.log("Server response in google:", data)
            if(data.verified){
                setUserSessionToken(data.token, data.expire);
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
  