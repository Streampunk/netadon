var http = require('http');
var net = require('net');
var netadon = require('../../netadon');
var argv = require('yargs')
  .default('h', 'localhost')
  .default('p', 5432)
  .default('t', 1)
  .default('n', 10)
  .number(['p', 't', 'n'])
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var options = {
  keepAlive: true,
  maxSockets: 10
};

var agent = new http.Agent(options);
agent.createConnection = function (options, cb) {
  var socket = net.createConnection(options, cb);
  socket.on('connect', () => {
    netadon.setSocketRecvBuffer(socket, 65535);
    netadon.setSocketSendBuffer(socket, 65535);
  });
  return socket;
};

var total = 0;
var tally = 0;

function runNext(x, tally, total) {
  http.get({
    agent : agent,
    hostname: argv.h,
    port: argv.p,
    path: '/essence'
  }, (res) => {
    // console.log(`Got response: ${res.statusCode}`);
    // consume response body
    var startTime = process.hrtime();
    var count = 0;
    res.on('data', (x) => {
      count++;// console.log(x.length);
    });
    res.on('end', () => {
      total++;
      tally += process.hrtime(startTime)[1]/1000000;
      console.log("No more data!", tally / total, total, x, count);
      if (total < argv.n) runNext(x, tally, total);
    });
  }).on('error', (e) => {
    console.log(`Got error: ${e.message}`);
  });
}

for ( var x = 0 ; x < argv.t ; x ++) {
  runNext(x, tally, total);
}
