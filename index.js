const express = require('express');
const { spawn } = require('child_process');
const admin = require('firebase-admin');
const path = require('path');
const serviceAccount = require('./firebaseSDK.json');
const cookieParser = require('cookie-parser');
const crypto = require('crypto');

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount)
});

const db = admin.firestore();
app.use(cookieParser());
app.set("view engine", "ejs");
app.use(express.static(path.join(__dirname, "public")));

const BYPASS_C_PROGRAM = false; 

const userCache = new Map();
let cprogram = null;
let rl = null;
const pendingRequests = new Map();
let requestCounter = 0;

if (!BYPASS_C_PROGRAM) {
  // Spawn C program with proper error handling
  cprogram = spawn("./app", [], {
    stdio: ['pipe', 'pipe', 'pipe']
  });

  cprogram.on('error', (err) => {
    console.error('Failed to start C program:', err);
  });

  cprogram.stderr.on('data', (data) => {
    console.error(`C program stderr: ${data}`);
  });

  // Create a more reliable communication mechanism
  const readline = require("readline");
  rl = readline.createInterface({ 
    input: cprogram.stdout,
    terminal: false
  });

  // Process responses from C program
  rl.on("line", (line) => {
    console.log(`Received from C program: ${line}`);
    
    // Try to find an unresolved request
    if (pendingRequests.size > 0) {
      const [id, { resolve }] = Array.from(pendingRequests.entries())[0];
      pendingRequests.delete(id);
      resolve(line);
    }
  });
}

function sendCommandToCProgram(command, timeout = 5000) {
  // If bypassing C program, return a mock response
  if (BYPASS_C_PROGRAM) {
    console.log(`[BYPASS MODE] Mock C program command: ${command}`);
    
    // Mock responses based on command type
    if (command.startsWith('addUser')) {
      return Promise.resolve("User added successfully");
    } else if (command.startsWith('tokenExists')) {
      // Always return false for tokenExists in bypass mode
      return Promise.resolve("User found : false");
    } else {
      return Promise.resolve("output : chalo");
    }
  }
  
  return new Promise((resolve, reject) => {
    const id = requestCounter++;
    const timer = setTimeout(() => {
      if (pendingRequests.has(id)) {
        pendingRequests.delete(id);
        reject(new Error(`Request timed out after ${timeout}ms: ${command}`));
      }
    }, timeout);
    
    pendingRequests.set(id, { 
      resolve: (result) => {
        clearTimeout(timer);
        resolve(result);
      },
      command
    });
    
    console.log(`Sending to C program: ${command}`);
    
    try {
      cprogram.stdin.write(command + "\n");
    } catch (err) {
      clearTimeout(timer);
      pendingRequests.delete(id);
      reject(new Error(`Failed to send command: ${err.message}`));
    }
  });
}

const handleRequestTimeout = (req, res, next) => {
  const requestTimeout = setTimeout(() => {
    if (!res.headersSent) {
      res.status(500).send("Request timed out");
    }
  }, 10000); // 10 second timeout
  
  // Store the timeout in the request object
  req.requestTimeout = requestTimeout;
  
  // Override end method to clear the timeout
  const originalEnd = res.end;
  res.end = function(...args) {
    clearTimeout(req.requestTimeout);
    return originalEnd.apply(this, args);
  };
  
  next();
};

app.use(handleRequestTimeout);

async function getUserData(userToken) {
  // Check cache first
  if (userCache.has(userToken)) {
    return userCache.get(userToken);
  }
  
  // Get from database
  const userDoc = await db.collection("tokens").doc(userToken).get();
  
  if (userDoc.exists) {
    const userData = userDoc.data();
    // Store in cache for future requests
    userCache.set(userToken, userData);
    return userData;
  }
  
  return null;
}

async function createNewUser(userToken) {
  const userData = {
    uuid: userToken,
    pref: "default",
    rated_movies: 0,
    liked_movies: 0,
    likes: [],
    ratings: []
  };
  
  await db.collection("tokens").doc(userToken).set(userData);
  userCache.set(userToken, userData);

  if (!BYPASS_C_PROGRAM) {
    sendCommandToCProgram(`addUser ${userToken} default 0 0`)
      .then(response => console.log(`Added user to C program: ${response}`))
      .catch(err => console.error("Error adding user to C program:", err));
  }
  

  setTimeout(() => {
    console.log("fduhf")
  }, 0);

  return userData;
}

app.get("/", async (req, res) => {
  try {
    let userToken = req.cookies.tokens;
    let userData = null;

    if (!userToken) {
      userToken = crypto.randomUUID();
      res.cookie("tokens", userToken, { maxAge: 31536000000 });
      
      userData = await createNewUser(userToken);
    } 
    else {
      userData = await getUserData(userToken);
      
      if (!userData) {
        userData = await createNewUser(userToken);
      }
    }
    
    if (req.headers.accept && req.headers.accept.includes('application/json')) {
      res.json({ user: userData });
    } else {
      
      res.send(`
        <!DOCTYPE html>
        <html lang="en">
        <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>User Profile</title>
          <style>
            body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }
            .container { max-width: 800px; margin: 0 auto; }
            pre { background: #f4f4f4; padding: 10px; border-radius: 5px; }
          </style>
        </head>
        <body>
          <div class="container">
            <h1>Welcome, User</h1>
            <h3>Your User ID: ${userData.uuid}</h3>
            <p>Preference: ${userData.pref}</p>
            <p>Liked Movies: ${userData.liked_movies}</p>
            <p>Rated Movies: ${userData.rated_movies}</p>
            <h3>Your Data:</h3>
            <pre>${JSON.stringify(userData, null, 2)}</pre>
          </div>
        </body>
        </html>
      `);
    }
    
  } catch (err) {
    console.error("Error processing request:", err);
    
    if (!res.headersSent) {
      res.status(500).send("Internal server error");
    }
  }
});

app.get("/explore", (req, res) => {
  try {
    
    res.send(`
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Explore</title>
        <style>
          body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }
          .container { max-width: 800px; margin: 0 auto; }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>Explore Page</h1>
          <p>This is the explore page content.</p>
        </div>
      </body>
      </html>
    `);
  } catch (err) {
    console.error("Error rendering explore page:", err);
    if (!res.headersSent) {
      res.status(500).send("Error loading explore page");
    }
  }
});

app.use((req, res) => {
  res.status(404).send("Page not found");
});

app.use((err, req, res, next) => {
  console.error("Unhandled error:", err);
  if (!res.headersSent) {
    res.status(500).send("Something went wrong");
  }
});

const server = app.listen(3000, () => {
  console.log("Server running on port 3000");
  console.log(`C Program Mode: ${BYPASS_C_PROGRAM ? 'BYPASSED' : 'ENABLED'}`);
});

server.on('error', (err) => {
  console.error("Server error:", err);
});

process.on('SIGTERM', () => {
  console.log('SIGTERM received, shutting down');
  
  server.close(() => {
    console.log('Server closed');
    
    if (cprogram) {
      cprogram.kill();
      console.log('C program terminated');
    }
    
    process.exit(0);
  });
});