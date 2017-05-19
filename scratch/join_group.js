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

var dgram = require('dgram');
var argv = require('yargs')
  .default('p', 6789)
  .default('a', '234.5.6.7')
  .number('p')
  .usage('Join a multicast group. Useful for wireshark testing.\n' +
    'Usage: $0 ')
  .help()
  .describe('a', 'Address of multicast group to join.')
  .describe('p', 'Port of multicast group to join.')
  .describe('i', 'Multicast interface.')
  .argv;

var socket = dgram.createSocket('udp4');

socket.addMembership(argv.a, argv.i);

socket.on('listening', () => {
  console.log('Multicast group subscribed and listening.');
});

socket.on('error', console.error);

socket.bind(argv.p);
