const express = require('express')
const { spawn } = require('child_process');
const admin = require('firebase-admin')
const path = require('path');
const serviceAccount = require('./firebaseSDK.json');
const cookieParser = require('cookie-parser');

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount)
});

const db = admin.firestore();
const app = express();
app.use(cookieParser());

app.set("view engine" , "ejs");
app.use(express.static(path.join(__dirname , "public")));

const cProcess = spawn('./cache_server', [], {
  stdio: ['pipe', 'pipe', 'pipe'] // stdin, stdout, stderr
});

// Request tracking for async communication with C program
const pendingRequests = new Map();

// Handle stdout from C process
cProcess.stdout.on('data', (data) => {
  const output = data.toString().trim();
  console.log('Raw C output:', output);
  
  try {
    // Try to parse the response from C program
    // Expected format: "requestId:JSON_DATA"
    const firstColonIndex = output.indexOf(':');
    if (firstColonIndex !== -1) {
      const requestId = output.substring(0, firstColonIndex);
      const jsonData = output.substring(firstColonIndex + 1);
      
      if (pendingRequests.has(requestId)) {
        const { resolve } = pendingRequests.get(requestId);
        pendingRequests.delete(requestId);
        
        try {
          const parsedData = JSON.parse(jsonData);
          resolve(parsedData);
        } catch (e) {
          resolve(jsonData); // If not valid JSON, return raw data
        }
      }
    }
  } catch (err) {
    console.error('Error processing C output:', err);
  }
});

cProcess.stderr.on('data', (data) => {
  console.error('C process error:', data.toString());
});

// Handle C process exit
cProcess.on('close', (code) => {
  console.error(`C process exited with code ${code}`);
  // You might want to restart the process here
});

// Function to send request to C program and await response
function sendToCProgram(command, data = {}) {
  return new Promise((resolve, reject) => {
    const requestId = uuidv4();
    
    // Store the resolve function with the request ID
    pendingRequests.set(requestId, { resolve, reject });
    
    // Format: "requestId:command:JSON_DATA\n"
    const requestData = `${requestId}:${command}:${JSON.stringify(data)}\n`;
    
    // Send to C program's stdin
    cProcess.stdin.write(requestData);
    
    // Set timeout to prevent hanging requests
    setTimeout(() => {
      if (pendingRequests.has(requestId)) {
        pendingRequests.delete(requestId);
        reject(new Error('Request to C program timed out'));
      }
    }, 5000); // 5 seconds timeout
  });
}


app.get("/" , async (req, res) => {
  const snapshot = await db.collection("Movies").get();
  let userToken = req.cookies.tokens;
  doc
  if(!userToken) {
    userToken = crypto.randomUUID();
    res.cookie("tokens", userToken, { maxAge: 31536000000 }); // 1 year in ms
    
    await db.collection("tokens").doc(userToken).set({
      uuid : userToken,
      pref : "default",
      likes : []
    });
  }
  
  const docRef = db.collection("tokens").doc(userToken);
  const doc = await docRef.get();
  const data = doc.data();

  const cprocess = spawn("./app", [data.uuid, data.pref]);  
  let result = "";

  cprocess.stdout.on('data', (data) => {
    result += data.toString();
  });

  cprocess.on('close', (code) => {
    console.log(`child process exited with code ${code}`);
    res.send(result); 
  });
});

app.get("/get" , (req, res) => {
  const cprocess = spawn("./app", []);  
  let result = "";

  cprocess.stdout.on('data', (data) => {
    result += data.toString();
  });

  cprocess.on('close', (code) => {
    console.log(`child process exited with code ${code}`);
    res.send(result); 
  });
})

app.listen(3000, () => {
  console.log("server running on port 3000");
});