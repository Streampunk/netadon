var udpPort = require('../../netadon');
var fs = require('fs');

process.env.UV_THREADPOOL_SIZE = 42;

var frame = fs.readFileSync('./essence/frame3.pgrp');

var soc = udpPort.createSocket('udp4');
soc.on('error', (err) => {
  //console.log(`server error: ${err}`);
});

var begin = process.hrtime();
var total = +process.argv[2];
var count = 0;

//soc.bind(6789, "192.168.1.12", () => {

for ( var y = 0 ; y < total  ; y++ ) {
  var startTime = process.hrtime();
  var frames = [];
  for ( var x = 0 ; x < frame.length ; x += 1440 ) {
    (function (offset, fnum) {
      frame.writeUInt8(0x80, offset);
      frame.writeUInt8(96, offset+1);
      frame.writeInt32LE(fnum * 3600 + offset / 1440, offset+2);
      // console.log('writing index', fnum, offset, fnum * 3600 + offset / 1440);
      frames.push(Buffer.from(frame.slice(offset, offset + 1440)));
    })(x, y);
  }

  soc.send(frames, 6789, '234.5.6.7', (err) => {
    if (err)
      console.log(`send error: ${err}`);
    count++;
    console.log(count, process.hrtime(begin));
    if (count === total) soc.close();
  });

  console.log('Finished', y, process.hrtime(begin));
}

//});

process.on('exit', () => {
  var totalTime = process.hrtime(begin);
  console.log(totalTime[0] + "." + totalTime[1]);
});
