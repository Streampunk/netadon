// Experimental only

var cluster = require('cluster');
var express = require('express');
var numCPUs = require('os').cpus().length;

cluster.schedulingPolicy = cluster.SCHED_RR;

var count = 0;

if (cluster.isMaster) {
  for ( var i = 0 ; i < numCPUs ; i++) {
    console.log(`Forking for CPU ${i}/${numCPUs}`);
    cluster.fork();

  }
  cluster.on('message', (worker, message, handle) => {
    console.log('Master from ' + worker.id + ': ' + message);
  });
} else {
  var app = express();

  (getter(app, cluster.worker.id))();

  app.listen(8080);
}

function getter(app, id) {
  var frank = 'Herbert' + id;
  return function () {
    console.log(frank);
    app.get('/', function (req, res) {
      console.log(frank);
      process.send('Bump ' + frank + ' ' + count);
      res.send(frank + count++);
    });
  }
}
