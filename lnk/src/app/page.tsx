import Image from 'next/image'
import { Inter } from 'next/font/google'
import styles from './page.module.css'


const inter = Inter({ subsets: ['latin'] })



export default function Home() {
  return (
  
    <main className={styles.main}>

      <script src="https://apis.google.com/js/platform.js" async defer></script>
      <script> import { GoogleUser, BasicProfile } from 'gapi.auth2';</script>
      <script>
        function onSignIn(googleUser) {
          const profile: BasicProfile = googleUser.getBasicProfile();
          console.log('ID: ' + profile.getId()); // Do not send to your backend! Use an ID token instead.
          console.log('Name: ' + profile.getName());
          console.log('Image URL: ' + profile.getImageUrl());
          console.log('Email: ' + profile.getEmail()); // This is null if the 'email' scope is not present.
        }

      </script>
      <meta name="google-signin-client_id" content="228928618151-a4lio74eijrst1fomvd46thcisdclqan.apps.googleusercontent.com"></meta>
      <div className="g-signin2" data-onsuccess="onSignIn"></div>

      <div className={styles.description}>
       
        <div className={styles.center}>
          Sign in
        </div>


      </div>

      <div className={styles.grid}>
      </div>
    </main>

    
  )
  

}



