# Persistent Key-Value Store Implementation

A Memcached-compatible server with disk persistence features:

##Protocol Implementation

- TCP socket server on port 9889
- Supports simplified Memcached ASCII protocol:
```set <key> <value-size-bytes>\r\n
  <value>\r\n

  get <key>\r\n
```

## Key Features

- File-system backed storage (data.db)
- Concurrency control with random delays (0-1s)
- Client testing harness (2+ concurrent clients)

## Usage

### Start server
```./kv_server --port 9889 --datafile data.db```

### Test client
```
./kv_client set foo "bar"
./kv_client get foo
```
