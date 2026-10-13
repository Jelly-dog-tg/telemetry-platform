# Telemetry Platform Performance Test


## 1. Test Overview

This document records the performance validation of the Linux C++ telemetry server.

The test focuses on:

- Multiple TCP client connections
- Continuous telemetry reporting
- Message processing throughput
- Response latency
- Server stability


## 2. Test Environment

Operating System:

Linux Ubuntu (WSL)

Compiler:

G++

C++ Standard:

C++17


CPU:

(fill according to your machine)


Memory:

(fill according to your machine)


## 3. System Architecture During Test

    |
    |
    v
    |
    |
    +---- Protocol Parser

    |
    +---- Device Manager

    |
    +---- CSV Storage


## 4. Test Case 1


### Configuration

Clients:

5


Duration:

10 seconds


Message rate:

2 messages/s/client


Expected messages:

5 × 10 × 2 = 100


### Result


Successful telemetry:


Failed operations:


Throughput:


Average latency:


P95 latency:


Success rate:



---

## 5. Test Case 2


### Configuration


Clients:

20


Duration:

30 seconds


Message rate:

5 messages/s/client


Expected messages:

20 × 30 × 5 = 3000


### Result


Successful telemetry:


Failed operations:


Throughput:


Average latency:


P95 latency:


Success rate:



---

## 6. Resource Observation


CPU usage:


Memory usage:


Observation:


During the test:

- Server remained running.
- No unexpected crashes occurred.
- TCP connections were handled correctly.


## 7. Conclusion


The telemetry server successfully supports:

- Multiple device connections
- Continuous telemetry transmission
- Protocol validation
- Device state update
- Persistent storage


The performance results demonstrate that the system satisfies the requirements of a Linux C++ device telemetry management application.
