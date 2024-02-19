### Total Order Broadcast

Implementation of total order broadcast using Lamport clocks

Application Layering:
- Network Layer: TCP sockets
- Data Layer: persistent multi-threaded memcache server
- Middleware: n servers listening on user-defined port range
- Application: clients with memcache like requests to middleware

Binaries:
- server: forks middleware child servers and runs the data layer in parent process
- client: request to server with set/get request 
    - port selection to one of the middleware is transparent
    - port selection is random and is handled on the client end

Testing:
- multiple clients with concurrent set/get requests to different middleware server
- client and server side logging
- comparison of log order on all processes
