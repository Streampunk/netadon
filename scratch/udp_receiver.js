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
var argv = require('yargs')
  .default('p', 6789)
  .default('a', '234.5.6.7')
  .default('rio', true)
  .number('p')
  .boolean('rio')
  .usage('Receive a test stream over UDP, counting the number of dropped packets.\n' +
    'Usage: $0 ')
  .help()
  .describe('a', 'Address of multicast group to join.')
  .describe('p', 'Port of multicast group to join.')
  .describe('i', 'Multicast interface card.')
  .describe('rio', 'Use Windows RIO for acceleration.')
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var udpPort = (argv.rio) ? netadon : dgram;

var soc = udpPort.createSocket('udp4');
soc.on('error', (err) => {
  console.log(`server error: ${err}`);
});

soc.bind(argv.p);

function isMulticast(x) {
  var firstByte = +x.slice(0, 3);
  return !isNaN(firstByte) && firstByte >= 224 && firstByte <= 239;
}

soc.on('listening', () => {
  if (isMulticast(argv.a)) soc.addMembership(argv.a, argv.i);
  console.log(`Bound to socket ${argv.p}, joined group ${argv.a} on interface ${argv.i}.`);
});

var pktCount = 0;
var seq = -1;
var discount = 0;
soc.on('message', (msg, rinfo) => {
  if (1440 != msg.length)
    console.log(msg.length);
  var pktSeq = msg.readInt32LE(2);
  if (pktSeq - seq > 1) {
    // console.log(`Discontinuity ${seq.toString(16)}->${pktSeq.toString(16)}`);
    discount += pktSeq - seq - 1;
  }
  seq = pktSeq;
  pktCount++;
});

var prev = -1;
setInterval(() => {
  console.log(pktCount, discount, pktCount + discount);
  if (prev != pktCount) { prev = pktCount; }
  else { pktCount = 0; prev = -1; discount = 0; }
}, 1000);
