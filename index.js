/* Copyright 2016 Streampunk Media Ltd.

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

'use strict';
var netAdon = require('bindings')('./Release/netadon');

const util = require('util');
const EventEmitter = require('events');

function UdpPort(ipType, packetSize, recvMinPackets, sendMinPackets, cb) {
  this.udpPortAdon = new netAdon.UdpPort(ipType, packetSize, recvMinPackets, sendMinPackets, function() {
    console.log('UdpPort exiting');
  });
  if (typeof cb === 'function')
    this.on('message', cb);

  EventEmitter.call(this);
}

util.inherits(UdpPort, EventEmitter);

UdpPort.prototype.addMembership = function(maddr, uaddr) {
  try {
    this.udpPortAdon.addMembership(maddr, uaddr);
  } catch (err) {
    this.emit('error', err);
  }
}
 
UdpPort.prototype.dropMembership = function(maddr, uaddr) {
  try {
    this.udpPortAdon.dropMembership(maddr, uaddr);
  } catch (err) {
    this.emit('error', err);
  }
}
 
UdpPort.prototype.setTTL = function(ttl) {
  try {
    this.udpPortAdon.setTTL(ttl);
  } catch (err) {
    this.emit('error', err);
  }
}

UdpPort.prototype.setMulticastTTL = function(ttl) {
  try {
    this.udpPortAdon.setMulticastTTL(ttl);
  } catch (err) {
    this.emit('error', err);
  }
}

UdpPort.prototype.setBroadcast = function(flag) {
  try {
    this.udpPortAdon.setBroadcast(flag);
  } catch (err) {
    this.emit('error', err);
  }
}
 
UdpPort.prototype.setMulticastLoopback = function(flag) {
  try {
    this.udpPortAdon.setMulticastLoopback(flag);
  } catch (err) {
    this.emit('error', err);
  }
}
 
UdpPort.prototype.bind = function(port, address, cb) {
  var bindPort = 0;
  var bindAddress = '';
  var bindCb;
  var curArg = 0;

  if (typeof arguments[curArg] === 'number') {
    bindPort = arguments[curArg];
    ++curArg;
  }
  if (typeof arguments[curArg] === 'string') {
    bindAddress = arguments[curArg];
    ++curArg;
  }
  if (typeof arguments[curArg] === 'function') {
    bindCb = arguments[curArg];
    ++curArg;
  }

  try {
    this.udpPortAdon.bind(bindPort, bindAddress, function(err, data, port, addr) {
      if (err)
        this.emit('error', err);
      else if (data)
        this.emit('message', data);
      else if ((typeof port === 'number') && (typeof addr === 'string')) {
        this.address = { port: port, address: addr };
        this.emit('listening');
        if (typeof bindCb === 'function') {
          bindCb(null);
        }
      }
      else
        this.emit('close');
    }.bind(this));
  } catch (err) {
    if (typeof bindCb === 'function')
      bindCb(err);
    else
      this.emit('error', err);
  } 
}

UdpPort.prototype.address = function() {
  return this.address;
}

UdpPort.prototype.send = function(data, offset, length, port, address, cb) {
  try {
    this.udpPortAdon.send(data, offset, length, port, address);
  } catch (err) {
    if (typeof cb === 'function')
      cb(err);
    else
      this.emit('error', err);
  } 
}

UdpPort.prototype.close = function(cb) {
  if (typeof cb === 'function')
    this.on('close', cb);

  try {
    this.udpPortAdon.close();
  } catch (err) {
    this.emit('error', err);
  } 
}

function netadon() {}
netadon.createSocket = function (ipType, packetSize, recvMinPackets, sendMinPackets, cb) {
  return new UdpPort (ipType, packetSize, recvMinPackets, sendMinPackets, cb);
} 

module.exports = netadon;
