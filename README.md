# Netadon

Netadon is a [Node.js](http://nodejs.org/) [addon](http://nodejs.org/api/addons.html) using Javascript and C++ to implement optimised UDP networking.
Currently only Windows hosts, UDP and IPv4 are supported. 

## Installation

Install [Node.js](http://nodejs.org/) for your platform. This software has been developed against the long term stable (LTS) release.

Netadon is designed to be `require`d to use from your own application to provide UDP networking.

    npm install --save netadon

Node.js addons use [libuv](http://libuv.org/) which by default supports up to 4 async threads in a threadpool for activities such as file I/O. These same threads are used by netadon and if you wish to use a number of the functions in one Node.js process then you will need to set the environment variable UV_THREADPOOL_SIZE to a suitable number before the first use of the libuv threadpool.

## Using netadon

To use netadon in your own application, `require` the module then create ports as required.
The functions are intended to follow the interface of the Node.js dgram module where possible.  There are some differences but it should be straightforward to test with either implementation   

```javascript
var netadon = require('netadon');
var udpPort = netadon.createSocket('udp4', packetSize, numPacketsRecv, numPacketsSend); 

udpPort.on('error', (err) => {
  console.log(`server error: ${err}`);
  udpPort.close();
});

udpPort.on('message', (data) => {
  console.log(`server data: ${data.length}`);
});

udpPort.on('listening', () => {
  var address = udpPort.address;
  console.log(`server listening ${address.address}:${address.port}`);
});

udpPort.bind(port, addr);
udpPort.send(buf, 0, buf.length, port, addr);
udpPort.close();
```
## Status, support and further development

Currently only Windows hosts, UDP and IPv4 are supported.

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

## License

This software is released under the Apache 2.0 license. Copyright 2016 Christine S MacNeill.
