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

var netadon = require('../../netadon');
var dgram = require('dgram');
var fs = require('fs');
var argv = require('yargs')
  .default('p', 6789)
  .default('a', '234.5.6.7')
  .default('rio', true)
  .default('f', 5184000)
  .default('n', 100)
  .default('s', 40)
  .default('ttl', 128)
  .number(['p', 'f', 'n', 's', 'ttl'])
  .boolean('rio')
  .usage('Send a test stream over UDP, counting the number of dropped packets.\n' +
    'Usage: $0 ')
  .help()
  .describe('a', 'Address of multicast group to join.')
  .describe('p', 'Port of multicast group to join.')
  .describe('i', 'Multicast interface card.')
  .describe('rio', 'Use Windows RIO for acceleration.')
  .describe('n', 'Number of frames to send.')
  .describe('s', 'Spacing between frames, measured in miliseconds.')
  .describe('ttl', 'Multicsat TTL.')
  .describe('f', 'Bytes per frame - default is 1080i25 10-bit.')
  .example('$0 -f 2304000 -i 10.11.12.13 -s 16.68333 for 720p60')
  .example('$0 -f 829440 -i 10.11.12.13 for 576i25')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var udpPort = (argv.rio) ? netadon : dgram;

var data = fs.readFileSync('./essence/frame3.pgrp');
var frame = Buffer.concat([data, data, data, data]).slice(0, argv.f);

var soc = udpPort.createSocket('udp4');

soc.on('error', (err) => {
  console.error(`server error: ${err}`);
});

soc.setMulticastTTL(argv.ttl);

var begin = process.hrtime();
var total = argv.n;
var count = 0;

//soc.bind(6789, "192.168.1.12", () => {
var y = 0;

function doit() {
  var diffTime = process.hrtime(begin);
  var diff = y * argv.s - (diffTime[0] * 1000 + diffTime[1] / 1000000|0);
  // console.log(y * 40, diffTime, diffTime[0] * 1000 + diffTime[1] / 1000000|0, diff);
  setTimeout(() => {
    sendFrame(y);
    y++;
    if (y < total) doit();
  }, diff > 0 ? diff|0 : 0);
}

function sendFrame(y) {
  var startTime = process.hrtime();
  var frames = [];
  for ( var x = 0 ; x < frame.length ; x += 1440 ) {
    (function (offset, fnum) {
      frame.writeUInt8(0x80, offset);
      frame.writeUInt8(96, offset+1);
      frame.writeInt32LE(fnum * 3600 + offset / 1440, offset+2);
      // console.log('writing index', fnum, offset, fnum * 3600 + offset / 1440);
      //frames.push(Buffer.from(frame.slice(offset, offset + 1440)));
      frames.push(frame.slice(offset, offset + 1440));
    })(x, y);
  }

  soc.send(frames, argv.p, argv.a, (err) => {
    if (err)
      console.log(`send error: ${err}`);
    count++;
    // console.log(count, process.hrtime(begin));
    if (count === total) soc.close();
  });

  // console.log('Finished', y, process.hrtime(begin));
}

//});
doit();

process.on('exit', () => {
  var totalTime = process.hrtime(begin);
  console.log(totalTime[0] + "." + totalTime[1]);
});
