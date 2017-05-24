/* Copyright 2017 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

var https = require('https');
var net = require('net');
var tls = require('tls');
var netadon = require('../../netadon');
var argv = require('yargs')
  .demandOption(['h'])
  .help()
  .default('p', 5432)
  .default('t', 1)
  .default('n', 100)
  .default('b', 65535)
  .default('N', true)
  .default('k', true)
  .default('i', 100)
  .default('s', 0)
  .number(['p', 't', 'n', 'b', 'i', 's'])
  .boolean(['N', 'k'])
  .usage('Pull frames from a server via HTTPS.\nUsage: $0 [options]')
  .describe('h', 'Hostname to connect to.')
  .describe('p', 'Port number to connect to.')
  .describe('t', 'Number of parrallel connections to use.')
  .describe('n', 'Number of frames to send.')
  .describe('b', 'TCP send and receive buffer sizes.')
  .describe('N', 'Disable Nagle\'s algorithm.')
  .describe('k', 'Use socket keep alive.')
  .describe('i', 'Interval between logging messages.')
  .describe('s', 'Spacing between frames, measured in miliseconds.')
  .example('$0 -h server -t 4 -n 1000')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var options = {
  keepAlive: argv.k,
  maxSockets: 10
};

var agent = new https.Agent(options);
agent.createConnection = function (options) {
  var socket = net.createConnection(options);
  socket.on('connect', () => {
    socket.setNoDelay(argv.N);
    netadon.setSocketRecvBuffer(socket, argv.b);
    netadon.setSocketSendBuffer(socket, argv.b);
  });
  options.socket = socket;
  return tls.connect(options);
}

var total = 0;
var tally = 0;

var begin = process.hrtime();
function runNext(x, tally, total, intervalTally) {
  var diffTime = process.hrtime(begin);
  var diff = ((total * argv.t + x) * argv.s) -
      (diffTime[0] * 1000 + diffTime[1] / 1000000|0);
  setTimeout(() => {
    var startTime = process.hrtime();
    https.get({
      agent: agent,
      rejectUnauthorized : false,
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
  }, diff > 0 ? diff|0 : 0);
}

for ( var x = 0 ; x < argv.t ; x ++) {
  runNext(x, tally, total);
}
