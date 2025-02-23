import Head from 'next/head'
import Image from 'next/image'
import Script from 'next/script'
import styles from '@/styles/Home.module.css'
import { useEffect, useRef, Suspense } from 'react'

import ServerInfo from '@/components/view_server'

export default function Home() {

  return (
    <>
      <Head>
        <title>Create Next App</title>
        <meta name="description" content="Generated by create next app" /> 
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <meta name="referrer" content="no-referrer-when-downgrade" />

        <Script src="./google.js"></Script>
      </Head>
      
      <main className={styles.main}>
        <ServerInfo/>
      </main>
    </>
  )
}
