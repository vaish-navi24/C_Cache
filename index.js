const express = require('express')
const { spawn } = require('child_process');
const admin = require('firebase-admin')
const path = require('path');
const serviceAccount = require('./firebaseSDK.json');
const cookieParser = require('cookie-parser');

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount)
});

const cprogram = spawn("./app");

isInitialised = false;

const db = admin.firestore();
app.use(cookieParser());

app.set("view engine" , "ejs");
app.use(express.static(path.join(__dirname , "public")));

const readline = require("readline");
const rl = readline.createInterface({ input: cprogram.stdout });

const requestQueue = [];
let isProcessing = false;

function sendCommandToCProgram(command) {
  return new Promise((resolve) => {
    requestQueue.push({ command, resolve });
    processQueue();
  });
}

function processQueue() {
  if (isProcessing || requestQueue.length === 0) return;

  isProcessing = true;
  const { command, resolve } = requestQueue.shift();

  const onLine = (line) => {
    rl.removeListener("line", onLine);
    isProcessing = false;
    resolve(line);
    processQueue();
  };

  rl.on("line", onLine);
  cprogram.stdin.write(command + "\n");
}

app.get("/", async (req, res) => {
  try {

    let userToken = req.cookies.tokens;
    let user;

    if(!userToken) {
      userToken = crypto.randomUUID();
      res.cookie("tokens", userToken, {maxAge : 31536000000});
      
      user = await db.collection("tokens").doc(userToken).set({
        uuid : userToken,
        pref : "default",
        rated_movies : 0,
        liked_movies : 0,
        likes : [],
        ratings : []
      });

      const createResponse = await sendCommandToCProgram(`addUser ${userToken} ${user.pref} ${liked_movies} ${rated_movies}`);
      return res.send(`User created: ${createResponse}`);      
    }

    else {
      const response = await sendCommandToCProgram(`tokenExists ${userToken}`);

      if (response.includes("false")) {
        const userDoc = await db.collection("tokens").doc(userToken).get();
        const user = userDoc.data();

        const createResponse = await sendCommandToCProgram(`addUser ${userToken} ${user.pref} ${user.liked_movies} ${user.rated_movies}`);
        return res.send(`User fetched from the db: ${createResponse}`);
      } 
      
      else {
        user = JSON.parse(response.trim());
        res.send(user);
      }
    }

  } catch (err) {
    console.error("Error communicating with C program:", err);
    res.status(500).send("Internal server error");
    
  }
});

app.listen(3000, () => {
  console.log("server running on port 3000");
});