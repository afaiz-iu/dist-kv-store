### Total Order Broadcast

Implementation of total order broadcast using Lamport clocks

Application Layering:
- Network Layer: TCP sockets
- Data Layer: persistent multi-threaded memcache server
- Middleware: 8 servers listening on a defined port range
- Application: clients with memcache like requests to middleware

Code:
- server_kv.c: data layer memcache server listening on port 4096 for incoming connection from middleware servers
- server_mid.c: forks middleware child servers. Each child handles incoming client and k-v server communication
- client.c: request to server with set/get request 
    - port selection to one of the middleware is transparent
    - port selection is random and is handled on the client end
- hashmap.c: implementaion of hashmap used in k-v store
- threadqueue.c: queue for handling multithreading. Each incoming client request spawns a new thread from a thread pool

Testing:
- multiple clients with concurrent set/get requests to different middleware server
- client and server side logging
- comparison of log order on all processes
