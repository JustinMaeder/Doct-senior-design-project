const express = require('express')
const app = express()
const eport = 3000
const Net = require('net');
// The port on which the server is listening.
const port = 8081;
var path = require('path');

var curPin

var recentJson
app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "doct.fbi.moe"); // update to match the domain you will make the request from
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

const server = new Net.Server();

server.listen(port, function() {
    console.log(`Server listening for connection requests on socket localhost:${port}`);
});

server.on('connection', function(socket) {
    console.log('A new connection has been established.');


    socket.on('data', function(chunk) {
        //if(!chunk.toString().length < 3) return
        console.log(`Data received from client: ${chunk.toString()}`);
	if(chunk.toString().includes('unitID')){
	    recentJson = chunk.toString()
    }
    });

    socket.on('end', function() {
        console.log('Closing connection with the client');
    });

    socket.on('error', function(err) {
        console.log(`Error: ${err}`);
    });
});




app.get('/api', (req, res) => res.send(recentJson));
app.get('/pin', (req, res) => res.send(val.toString()));
app.get('/loc', function(req, res) {
    res.sendFile(path.join(__dirname + '/t.html'));
});

app.get('/speedometer', function(req, res) {
    res.sendFile(path.join(__dirname + '/speed.html'));
});


app.get('/speed', function(req, res) {
recentObj = JSON.parse(recentJson.replace(/'/g, '"'));
res.send(recentObj.spd)
});
app.get('/locker', function(req, res) {
    res.sendFile(path.join(__dirname + '/f.html'));
});

app.listen(eport, () => console.log(`Example app listening on port ${eport}!`))

var val = Math.floor(1000 + Math.random() * 9000);
console.log("\n4 Digit PIN:",val.toString(),"\n");
