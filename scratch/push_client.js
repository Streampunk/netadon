var fs = require('fs');
var http = require('http');
var argv = require('yargs')
  .default('h', 'localhost')
  .default('p', 5432)
  .default('t', 1)
  .default('n', 100)
  .number(['p', 'n', 't'])
  .argv;

process.env.UV_THREADPOOL_SIZE = 42;

var frame = fs.readFileSync('./essence/frame3.pgrp');

function nextOne(x, tallyReq, tallyRes, total) {
  var options = {
    hostname: argv.h,
    port: argv.p,
    path: '/essence',
    method: 'POST',
    headers: {
      'Content-Type': 'application/octet-stream',
      'Content-Length': frame.length
    }
  };

  var startTime = process.hrtime();

  var req = http.request(options, (res) => {
    var chunks = 0;
    res.setEncoding('utf8');
    res.on('data', (chunk) => {
      // console.log(`BODY: ${chunk}`);
      chunks++;
    });
    res.on('end', () => {
      tallyRes += process.hrtime(startTime)[1]/1000000;
      if (total % 100 === 0) console.log(x, total, tallyReq/total, tallyRes/total);
      if (total < argv.n]) nextOne(x, tallyReq, tallyRes, total);
      else console.log('Finished', x, total, tallyReq/total, tallyRes/total, chunks);
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
    // if (total < +process.argv[3]) nextOne(x, tallyReq, tallyRes, total);
  });
}

for ( var x = 0 ; x < argv.t ; x++ ) {
  nextOne(x, 0, 0, 0);
}
