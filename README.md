<html>
<body>
<h1 align="center">wrk</h1>
<p align="center"><b>Modern HTTP Benchmarking Tool</b></p>
<table>
  <tr align="center">
    <td width="222px"><a href="https://github.com/wg/wrk/releases">Version: 4.0.1</a></td>
    <td width="222px"><a href="https://github.com/wg/wrk/blob/master/CHANGES">Changelog</a></td>
    <td width="222px"><a href="https://github.com/wg/wrk/blob/master/LICENSE">License</a></td>
    <td width="222px"><a href="https://github.com/wg/wrk/wiki">Wiki</a></td>
  </tr>
</table>
<table>
  <tr>
    <td align="center" width="888px"><b>ABOUT</b></td>
  </tr>
  <tr>
    <td>
      <p>wrk is a modern HTTP benchmarking tool capable of generating significant load when run on a single multi-core CPU. It combines a multithreaded design with scalable event notification systems such as epoll and kqueue.</p>
      <p>An optional LuaJIT script can perform HTTP request generation, response processing, and custom reporting. Details are available in SCRIPTING and several examples are located in ‘scripts/’.</p>
    </td>
  </tr>
</table>
<table>
  <tr>
    <td align="center" width="888px"><b>BENCHMARKING TIPS</b></td>
  </tr>
  <tr>
    <td>
      <p>The machine running wrk must have a sufficient number of ephemeral ports available and closed sockets should be recycled quickly. To handle the initial connection burst the server's listen(2) backlog should be greater than the number of concurrent connections being tested.</p>
      <p>A user script that only changes the HTTP method, path, adds headers or a body, will have no performance impact. Per-request actions, particularly building a new HTTP request, and use of response() will necessarily reduce the amount of load that can be generated.</p>
    </td>
  </tr>
</table>
<table>
  <tr>
    <td align="center" width="888px"><b>ACKNOWLEDGEMENTS</b></td>
  </tr>
  <tr>
    <td>
      <p>wrk contains code from a number of open source projects including the 'ae' event loop from redis, the nginx/joyent/node.js 'http-parser', and Mike Pall's LuaJIT. Please consult the <a href="https://github.com/wg/wrk/blob/master/NOTICE">NOTICE</a> file for licensing details.</p>
    </td>
  </tr>
</table>
</body>
</html>
