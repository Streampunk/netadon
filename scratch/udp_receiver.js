var udpPort = require('../../netadon');

process.env.UV_THREADPOOL_SIZE = 42;

var soc = udpPort.createSocket('udp4');
soc.on('error', (err) => {
  console.log(`server error: ${err}`);
});

soc.bind(6789, () => {
  soc.addMembership('234.5.6.7');
  console.log("socket bound");
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
