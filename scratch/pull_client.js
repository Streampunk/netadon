var http = require('http');
var net = require('net');
var netadon = require('../../netadon');
var argv = require('yargs')
  .demandOption(['h'])
  .help('help')
  .default('p', 5432)
  .default('t', 1)
  .default('n', 100)
  .default('b', 65535)
  .default('N', true)
  .default('k', true)
  .default('i', 100)
  .number(['p', 't', 'n', 'b', 'i'])
  .boolean(['N', 'k'])
  .usage('Pull frames from a server via HTTP.\n' +
    'Usage: $0 [options]')
  .describe('h', 'Hostname to connect to.')
  .describe('p', 'Port number to connect to.')
  .describe('t', 'Number of parrallel connections to use.')
  .describe('n', 'Number of frames to send.')
  .describe('b', 'TCP send and receive buffer sizes.')
  .describe('N', 'Disable Nagle\'s algorithm.')
  .describe('k', 'Use socket keep alive.')
  .describe('i', 'Interval between logging messages.')
  .example('$0 -h server -t 4 -n 1000')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var options = {
  keepAlive: argv.k,
  maxSockets: 10
};

var agent = new http.Agent(options);
agent.createConnection = function (options) {
  var socket = net.createConnection(options);
  socket.on('connect', () => {
    socket.setNoDelay(argv.N);
    netadon.setSocketRecvBuffer(socket, argv.b);
    netadon.setSocketSendBuffer(socket, argv.b);
  });
  return socket;
}

var total = 0;
var tally = 0;

function runNext(x, tally, total, intervalTally) {
  var startTime = process.hrtime();
  http.get({
    agent : agent,
    hostname: argv.h,
    port: argv.p,
    path: '/essence'
  }, (res) => {
    var count = 0;
    res.on('data', (x) => {
      count++;
    });
    res.on('end', () => {
      total++;
      var frameTime = process.hrtime(startTime)[1]/1000000;
      tally += frameTime;
      if (total % argv.i === 0) {
        console.log(`Thread ${x}: total = ${total}, avg = ${tally/total}, intervalAvg = ${intervalTally/argv.i}, chunks/frame = ${count}`);
        intervalTally = 0.0;
      }
      if (total < argv.n) {
        intervalTally += frameTime;
        runNext(x, tally, total, intervalTally);
      } else {
        console.log(`Finished thread ${x}: total = ${total}, avg = ${tally/total}, chunks/frame = ${count}`);
      }
    });
  }).on('error', (e) => {
    console.log(`Got error: ${e.message}`);
  });
}

for ( var x = 0 ; x < argv.t ; x ++) {
  runNext(x, tally, total, 0.0);
}
