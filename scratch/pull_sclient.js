var https = require('https');

process.env.UV_THREADPOOL_SIZE = 42;

var total = 0;
var tally = 0;

function runNext(x, tally, total) {
  https.get({
    rejectUnauthorized : false,
    hostname: '169.254.113.214',
    port: 8901,
    path: '/essence'
  }, (res) => {
    console.log(`Got response: ${res.statusCode}`);
    // consume response body
    var startTime = process.hrtime();
    var count = 0;
    res.on('data', (x) => {
      count++;// console.log(x.length);
    });
    res.on('end', () => {
      total++;
      tally += process.hrtime(startTime)[1]/1000000;
      console.log("No more data!", tally / total, total, x, count);
      if (total < 100000) runNext(x, tally, total);
    });
  }).on('error', (e) => {
    console.log(`Got error: ${e.message}`);
  });
}

for ( var x = 0 ; x < 8 ; x ++) {
  runNext(x, tally, total);
}
