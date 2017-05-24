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

var express = require('express');
var fs = require('fs');
var bp = require('body-parser');
var https = require('https');
var netadon = require('../../netadon');
var argv = require('yargs')
  .default('p', 5432)
  .default('b', 65535)
  .default('N', true)
  .default('f', 5184000)
  .number(['p', 'b', 'f'])
  .boolean('N')
  .help()
  .usage('HTTPS server for testing frame movement.\n' +
    'Usage: $0 [options]')
  .describe('p', 'Port number to listen on.')
  .describe('b', 'TCP send and receive buffer size.')
  .describe('N', 'Disable Nagle\'s algorithm')
  .describe('f', 'Bytes per frame - default is 1080i25 10-bit.')
  .example('$0 -f 2304000 for 720p60')
  .example('$0 -f 829440 for 576i25')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var data = fs.readFileSync('./essence/frame3.pgrp');
var frame = Buffer.concat([data, data, data, data]).slice(0, argv.f);

var app = express();
app.use(bp.raw({ limit : 6000000 }));

app.get('/essence', function (req, res) {
  res.send(frame);
});

var pushCount = 0;
var startTime = process.hrtime();

app.post('/essence', function (req, res) {
  if (pushCount === 0) startTime = process.hrtime();
  // console.log(pushCount++, process.hrtime(startTime), req.body.length);
  res.json({ wibble : 'wibble' });
});

var server = https.createServer({
  key : fs.readFileSync('./gonzales.pem'),
  cert : fs.readFileSync('./gonzales-cert.pem')
}, app).listen(argv.p);

server.on('listening', () => {
  console.log(`Gonzales HTTPS server listining on port ${argv.p}.`);
});

var consPerTen = 0;
var date = () => { return new Date().toISOString(); }
server.on('connection', (s) => {
  if (connsPerTen < 10) {
    setImmediate(() => {
      console.log(`${date()}: Gonzales HTTP server new connection ${s.address().address}.`);
      connsPerTen++;
    });
  } else if (connsPerTen == 10) {
    setImmediate(() => {
      console.log('No more connection messages until 10 seconds has passed.');
      connsPerTen++;
    });
  }
  s.setNoDelay(argv.N);
  netadon.setSocketRecvBuffer(s, argv.b);
  netadon.setSocketSendBuffer(s, argv.b);
});
server.on('error', console.error);

setInterval(() => { connsPerTen = 0; }, 10000);
