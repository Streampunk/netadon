var express = require('express');
var fs = require('fs');
var bp = require('body-parser');
var https = require('https');
var netadon = require('../../netadon');
var argv = require('yargs')
  .default('p', 8901)
  .default('b', 65535)
  .default('N', true)
  .number(['p', 'b'])
  .boolean('N')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var frame = fs.readFileSync('./essence/frame3.pgrp');

var app = express();
app.use(bp.raw({ limit : 6000000 }));

app.get('/essence', function (req, res) {
  res.send(frame);
});

var pushCount = 0;
var startTime = process.hrtime();

app.post('/essence', function (req, res) {
  if (pushCount === 0) startTime = process.hrtime();
  console.log(pushCount++, process.hrtime(startTime), req.body.length);
  res.json({ wibble : 'wibble' });
});

var server = https.createServer({
  key : fs.readFileSync('./gonzales.pem'),
  cert : fs.readFileSync('./gonzales-cert.pem')
}, app).listen(argv.p);

server.on('listening', () => {
  console.log(`Gonzales HTTPS server listining on port ${argv.p}.`);
});

var date = () => { return new Date().toISOString(); }
server.on('connection', (s) => {
  console.log(`${date()}: Gonzales HTTPS server new connection ${s.address().address}.`);
  s.setNoDelay(argv.N);
  netadon.setSocketRecvBuffer(s, argv.b);
  netadon.setSocketSendBuffer(s, argv.b);
});
server.on('error', console.error);
