### Totally Ordered Broadcast System

Implementation of Lamport timestamp-based ordering:

## Components

1. Middleware Layer
   - Intercepts TCP messages
   - Applies logical clock timestamps
2. Delivery Guarantees
   - Total order preservation
   - Crash recovery mechanism
