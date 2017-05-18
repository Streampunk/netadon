var http = require('http');
var net = require('net');
var netadon = require('../../netadon');
var argv = require('yargs')
  .default('h', 'localhost')
  .default('p', 5432)
  .default('t', 1)
  .default('n', 100)
  .default('b', 65535)
  .default('N', true)
  .default('k', true)
  .default('i', 100)
  .number(['p', 't', 'n', 'b', 'i'])
  .boolean('N')
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

function runNext(x, tally, total) {
  var startTime = process.hrtime();
  http.get({
    agent : agent,
    hostname: argv.h,
    port: argv.p,
    path: '/essence'
  }, (res) => {
    // console.log(`Got response: ${res.statusCode}`);
    // consume response body
    var count = 0;
    res.on('data', (x) => {
      count++;// console.log(x.length);
    });
    res.on('end', () => {
      total++;
      tally += process.hrtime(startTime)[1]/1000000;
      if (total % argv.i === 0)
        console.log(`Thread ${x}: total = ${total}, avg = ${tally/total}, chunks/frame = ${count}`);
      if (total < argv.n) {
        runNext(x, tally, total)
      } else {
        console.log(`Finished thread ${x}: total = ${total}, avg = ${tally/total}, chunks/frame = ${count}`);
      }
    });
  }).on('error', (e) => {
    console.log(`Got error: ${e.message}`);
  });
}

for ( var x = 0 ; x < argv.t ; x ++) {
  runNext(x, tally, total);
}
