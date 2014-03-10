//this is for thread_number.lua
var http = require('http');

http.createServer(function(req, res){
	console.log(req.headers['x-thread']);
	res.end(req.headers['x-thread']);
}).listen(1234);

console.log('now run: $ ./wrk -t 4 -d 1s -s scripts/thread_number.lua "http://127.0.0.1:1234/"')