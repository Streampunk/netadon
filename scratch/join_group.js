var dgram = require('dgram');
var argv = require('yargs')
  .default('p', 6789)
  .default('a', '234.5.6.7')
  .demandOption(['i'])
  .number('a')
  .usage('Join a multicast group. Useful for wireshark testing.\n' +
    'Usage: $0 ')
  .describe('a', 'Address of multicast group to join.')
  .describe('g', 'Port of multicast group to join.')
  .describe('i', 'Multicast interface.')
  .argv;

var socket = dgram.createSocket('udp4');

var addr = process.argv[2];
var port = +process.argv[3];

socket.addMembership(argv.a, argv.i);

socket.on('listening', () => {
  console.log('Multicast group subscribed and listening.');
});

socket.on('error', console.error);

socket.bind(argv.p);
