var express = require('express');
var fs = require('fs');
var bp = require('body-parser');
var argv = require('yargs')
  .default('p', 5432)
  .number('p')
  .argv;
process.env.UV_THREADPOOL_SIZE = 42;

var app = express();
app.use(bp.raw({ limit : 6000000 }));

app.get('/essence', function (req, res) {
  res.send(frame);
});

var pushCount = 0;
var startTime = process.hrtime();

app.post('/essence', function (req, res) {
  if (pushCount === 0) startTime = process.hrtime();
  console.log(pushCount++, process.hrtime(startTime), req.body.length);
  res.json({ wibble : 'wibble' });
});

app.listen(argv.p, function() {
  console.log(`Gonzales listening on port ${argv.p}.`);
});
