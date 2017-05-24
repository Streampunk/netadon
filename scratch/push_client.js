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

var fs = require('fs');
var net = require('net');
var http = require('http');
var netadon = require('../../netadon');
var argv = require('yargs')
  .demandOption(['h'])
  .default('p', 5432)
  .default('t', 1)
  .default('n', 100)
  .default('i', 100)
  .default('f', 5184000)
  .default('k', true)
  .default('b', 65535)
  .default('N', true)
  .default('s', 0)
  .boolean(['k', 'N'])
  .number(['p', 'n', 't', 'i', 'f', 'b', 's'])
  .help()
  .usage('Push frames to a server via HTTP.\n' +
    'Usage: $0 [options]')
  .describe('h', 'Hostname to connect to.')
  .describe('p', 'Port number to connect to.')
  .describe('t', 'Number of parrallel connections to use.')
  .describe('n', 'Number of frames to send.')
  .describe('i', 'Interval between logging messages.')
  .describe('f', 'Bytes per frame - default is 1080i25 10-bit.')
  .describe('k', 'Use HTTP keep alive.')
  .describe('b', 'TCP send and receive buffer sizes.')
  .describe('N', 'Disable Nagle\'s algorithm.')
  .describe('s', 'Spacing between frames, measured in miliseconds.')
  .example('$0 -h server -t 4 -n 1000', 'send 1000 frames on 4 threads to server')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var data = fs.readFileSync('./essence/frame3.pgrp');
var frame = Buffer.concat([data, data, data, data]).slice(0, argv.f);

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

var begin = process.hrtime();

function nextOne(x, tallyReq, tallyRes, total, intervalTally) {
  var diffTime = process.hrtime(begin);
  var diff = ((total * argv.t + x) * argv.s) -
      (diffTime[0] * 1000 + diffTime[1] / 1000000|0);
  setTimeout(() => {
    var options = {
      agent: agent,
      hostname: argv.h,
      port: argv.p,
      path: '/essence',
      method: 'POST',
      headers: {
        'Content-Type': 'application/octet-stream',
        'Content-Length': frame.length
      }
    };

    var startTime = process.hrtime();

    var req = http.request(options, (res) => {
      var chunks = 0;
      res.setEncoding('utf8');
      res.on('data', (chunk) => {
        // console.log(`BODY: ${chunk}`);
        chunks++;
      });
      res.on('end', () => {
        var resTime = process.hrtime(startTime)[1]/1000000;
        tallyRes += resTime;
        intervalTally += resTime;
        if (total % argv.i === 0) {
          console.log(`Thread ${x}: total = ${total}, avgReq = ${tallyReq/total}, avgRes = ${tallyRes/total}, intervalAvg = ${intervalTally/argv.i}`);
          intervalTally = 0;
        }
        if (total < argv.n) nextOne(x, tallyReq, tallyRes, total, intervalTally);
        else console.log(`Finished thread ${x}: total = ${total}, avgReq = ${tallyReq/total}, avgRes = ${tallyRes/total}, intervalAvg = ${intervalTally/argv.i}`);
      })
    });

    req.on('error', (e) => {
      console.log(`problem with request: ${e.message}`);
    });

    // write data to request body
    req.write(frame);
    req.end();
    req.on('finish', () => {
      total++;
      var reqTime = process.hrtime(startTime)[1]/1000000;
      tallyReq += reqTime;
      // intervalTally += reqTime;
      // console.log("Finished writing", x, total, process.hrtime(startTime)[1]/1000000);
      // if (total < +process.argv[3]) nextOne(x, tallyReq, tallyRes, total);
    });
  }, diff > 0 ? diff|0 : 0);
}

for ( var x = 0 ; x < argv.t ; x++ ) {
  nextOne(x, 0, 0, 0, 0);
}
