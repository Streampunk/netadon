var dgram = require('dgram')

var socket = dgram.createSocket('udp4');

var addr = process.argv[2];
var port = +process.argv[3];

socket.addMembership(addr);


socket.on('listening', () => {
  console.log('Multicast group subscribed and listening.');
});

socket.on('error', console.error);

socket.on('message', console.log);

socket.bind(port);
