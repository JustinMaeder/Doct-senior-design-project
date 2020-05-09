var net = require('net');

var server = net.createServer(function(client) {

//    console.log('Client connect. Client local address : ' + client.localAddress + ':' + client.localPort + '. client remote address : ' + client.remoteAddress + ':' + client.remotePort);

    client.setEncoding('utf-8');

    client.setTimeout(10000);

    client.on('data', function (data) {
	console.log(data.toString().replace(/\r?\n|\r/g, " "))
	if(data.unitID){
//	console.log(data.unitID,batt,lat,long,spd)
		}

	    });

    client.on('end', function () {
        console.log('Client disconnect.');
        server.getConnections(function (err, count) {
            if(!err)
            {
                console.log("There are %d connections now. ", count);
            }else
            {
                console.error(JSON.stringify(err));
            }

        });
    });

    client.on('timeout', function () {
        console.log('Client request time out. ');
    })
});

server.listen(1337, function () {

    var serverInfo = server.address();

    var serverInfoJson = JSON.stringify(serverInfo);

    console.log('TCP server listen on address : ' + serverInfoJson);

    server.on('close', function () {
        console.log('TCP server socket is closed.');
    });

    server.on('error', function (error) {
        console.error(JSON.stringify(error));
    });

});
