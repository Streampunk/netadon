var fs = require('fs');
var https = require('https');

process.env.UV_THREADPOOL_SIZE = 42;

var frame = fs.readFileSync('./essence/frame3.pgrp');

var total = 0;
var tallyReq = 0;
var tallyRes = 0;

function nextOne(x, tallyReq, tallyRes, total) {
  var options = {
    rejectUnauthorized : false,
    hostname: '169.254.128.166',
    port: 8901,
    path: '/essence',
    method: 'POST',
    headers: {
      'Content-Type': 'application/octet-stream',
      'Content-Length': frame.length
    }
  };

  var startTime = process.hrtime();

  var req = https.request(options, (res) => {
    res.setEncoding('utf8');
    res.on('data', (chunk) => {
      // console.log(`BODY: ${chunk}`);
    });
    res.on('end', () => {
      tallyRes += process.hrtime(startTime)[1]/1000000;
      if (total % 100 === 0) console.log(x, total, tallyReq/total, tallyRes/total);
      if (total < +process.argv[3]) nextOne(x, tallyReq, tallyRes, total);
      else console.log('Finished', x, total, tallyReq/total, tallyRes/total);
    })
  });

  req.on('error', (e) => {
    console.log(`problem with request: ${e.message}`);
  });

  // write data to request body
  req.write(frame);
  req.end();
  req.on('finish', () => {
    total++;
    tallyReq += process.hrtime(startTime)[1]/1000000;
    // console.log("Finished writing", x, total, process.hrtime(startTime)[1]/1000000);
  });
}

for ( var x = 0 ; x < +process.argv[2] ; x++ ) {
  nextOne(x, 0, 0, 0);
}
