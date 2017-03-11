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

'use strict';
var netAdon = require('bindings')('./Release/netadon');

//var SegfaultHandler = require('segfault-handler');
//SegfaultHandler.registerHandler("crash.log");

var dgram = require('dgram');
const util = require('util');
const EventEmitter = require('events');

function UdpPort(options, cb, packetSize, recvMinPackets, sendMinPackets) {
  var curArg = 0;
  var optionsObj = { type:'udp4', reuseAddr:false };
  if (typeof arguments[curArg] === 'string') {
    optionsObj.type = arguments[curArg];
  } else if (typeof arguments[curArg] === 'object') {
    optionsObj = arguments[curArg];
  }
  ++curArg;

  var rioCb;
  var rioPacketSize = 1500;
  var rioRecvMinPackets = 16384;
  var rioSendMinPackets = 16384;

  if (typeof arguments[curArg] === 'function') {
    rioCb = arguments[curArg];
    ++curArg;
  }
  if (typeof arguments[curArg] === 'number') {
    rioPacketSize = arguments[curArg++];
    rioRecvMinPackets = arguments[curArg++];
    rioSendMinPackets = arguments[curArg++];
  }

  this.udpPortAdon = new netAdon.UdpPort(optionsObj, rioPacketSize, rioRecvMinPackets, rioSendMinPackets,
  (err, data) => {
    if (err)
      this.emit('error', err);
    else if (data)
      this.emit('message', data);
    else
      this.emit('close');
  },
  () => {
    console.log('UdpPort exiting');
  });
  if (typeof rioCb === 'function')
    this.on('message', rioCb);

  EventEmitter.call(this);
}

util.inherits(UdpPort, EventEmitter);

UdpPort.prototype.addMembership = function(maddr, optUaddr) {
  var uaddr = '';
  if (typeof arguments[1] === 'string')
    uaddr = optUaddr;

  try {
    this.udpPortAdon.addMembership(maddr, uaddr);
  } catch (err) {
    this.emit('error', err);
  }
}

UdpPort.prototype.dropMembership = function(maddr, optUaddr) {
  var uaddr = '';
  if (typeof arguments[1] === 'string')
    uaddr = optUaddr;

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
  var bindAddr = '';
  var bindCb;
  var curArg = 0;

  if (typeof arguments[curArg] === 'number') {
    bindPort = arguments[curArg];
    ++curArg;
  }
  if (typeof arguments[curArg] === 'string') {
    bindAddr = arguments[curArg];
    ++curArg;
  }
  if (typeof arguments[curArg] === 'function') {
    bindCb = arguments[curArg];
    ++curArg;
  }

  try {
    this.udpPortAdon.bind(bindPort, bindAddr, (err, port, addr) => {
      if (err)
        this.emit('error', err);
      else {
        this.bindAddress = { port: port, address: addr };
        this.emit('listening');
        if (typeof bindCb === 'function') {
          bindCb(null);
        }
      }
    });
  } catch (err) {
    if (typeof bindCb === 'function')
      bindCb(err);
    else
      this.emit('error', err);
  }
}

UdpPort.prototype.address = function() {
  return this.bindAddress;
}

UdpPort.prototype.send = function(data, offset, length, port, address, cb) {
  var sendOffset = 0;
  var sendLength = 0;
  var sendPort = 0;
  var sendAddr = '';
  var sendCb;
  var curArg = 1;

  if (!Array.isArray(data)) {
    sendOffset = arguments[curArg++];
    sendLength = arguments[curArg++];
  } else if (arguments.length > 4) { // temp support for offset and length being provided...
    console.log("udpPort send: offset and length should not be provided!!");
    curArg += 2;
  }

  sendPort = arguments[curArg++];
  sendAddr = arguments[curArg++];
  sendCb = arguments[curArg++]; // optional - may be undefined

  try {
    var bufArray;
    if (Buffer.isBuffer(data)) {
      bufArray = new Array(1);
      bufArray[0] = data;
    } else if (typeof data === 'string') {
      bufArray = new Array(1);
      bufArray[0] = Buffer.from(data);
    } else if (Array.isArray(data))
      bufArray = data;
    else
      throw ("Expected send buffer not found");

    this.udpPortAdon.send(bufArray, sendOffset, sendLength, sendPort, sendAddr, () => {
      var ba = bufArray.length; // protect bufArray from GC until callback has fired !!
      if (typeof sendCb === 'function')
        sendCb(null);
    });
  } catch (err) {
    if (typeof sendCb === 'function')
      sendCb(err);
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

netadon.setSocketRecvBuffer = function(socket, numBytes) {
  netAdon.setSocketRecvBuffer(socket._handle, numBytes);
}

netadon.setSocketSendBuffer = function(socket, numBytes) {
  netAdon.setSocketSendBuffer(socket._handle, numBytes);
}

netadon.createSocket = function (options, cb, packetSize, recvMinPackets, sendMinPackets) {
  try {
    var sock = new UdpPort (options, cb, packetSize, recvMinPackets, sendMinPackets);
    return sock;
  } catch (err) {
    console.log('Falling back to dgram sockets as no fast networking support on this platform.');
    var sock = dgram.createSocket(options, cb);
    if (process.version.startsWith('v4.')) {
      var simpleSend = sock.send;
      sock.send = function (data, offset, length, port, address, cb) {
        var sendOffset = 0;
        var sendLength = 0;
        var sendPort = 0;
        var sendAddr = '';
        var sendCb;
        var curArg = 1;

        if (!Array.isArray(data)) {
          sendOffset = arguments[curArg++];
          sendLength = arguments[curArg++];
        }
        sendPort = arguments[curArg++];
        sendAddr = arguments[curArg++];
        sendCb = arguments[curArg++]; // optional - may be undefined

        var bufArray;
        if (Buffer.isBuffer(data)) {
          bufArray = new Array(1);
          bufArray[0] = data;
        } else if (typeof data === 'string') {
          bufArray = new Array(1);
          bufArray[0] = Buffer.from(data);
        } else if (Array.isArray(data))
          bufArray = data;
        else
          throw ("Expected send buffer not found");

        // console.log("Sending from buffer of length", bufArray.length);
        bufArray.forEach((p) => {
          simpleSend.call(sock, p, sendOffset, sendLength?sendLength:p.length, sendPort, sendAddr, (err) => {
            if (err) sendCb(err);
          });
        });

        if (typeof sendCb === 'function')
          sendCb();
      }
    }
    return sock;
  }
}

module.exports = netadon;
