import Head from 'next/head'
import Image from 'next/image'
import Script from 'next/script'
import { Inter } from 'next/font/google'
import styles from '@/styles/Home.module.css'
import { useEffectm, useRef, Suspense } from 'react'
// import loadable from '@loadable/component'
import { GoogleLogin } from '@react-oauth/google'

import Google from './google'
import { GoogleOAuthProvider } from '@react-oauth/google'


const inter = Inter({ subsets: ['latin'] })

export default function Home() {


    function GoogleButton(){
      return (
        <div className="GoogleButton">
          <header className="App-header">
            <GoogleOAuthProvider clientId="228928618151-a4lio74eijrst1fomvd46thcisdclqan.apps.googleusercontent.com">
              <Google />
            </GoogleOAuthProvider>
          </header>
        </div>
      );
    }

  return (
    <>
      <Head>
        <title>Create Next App</title>
        <meta name="description" content="Generated by create next app" /> 
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <meta name="referrer" content="no-referrer-when-downgrade" />

        <link rel="icon" href="/favicon.ico" />
        <script src="https://accounts.google.com/gsi/client" async defer></script>

        {/* <Script src="https://accounts.google.com/gsi/client"  async defer></Script> */} 
        <Script src="./google.js"></Script>
      </Head>
      
      <main className={styles.main}>
        <GoogleButton/>
      </main>
    </>
  )
}
