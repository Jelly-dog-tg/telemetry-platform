
# Linux C++ 多设备遥测与故障诊断平台

## 1. 项目简介

本项目使用 C++17 和 Linux Socket 实现一个多设备遥测数据接收与故障诊断服务。

客户端通过 TCP 发送设备编号、温度、在线状态和消息序号。服务器完成消息解析、数据校验、设备状态更新、温度告警及 CSV 持久化。

项目主要用于实践 Linux C++ 工程开发、网络通信、并发编程和自动化测试。

## 2. 核心功能

- 使用 TCP Socket 接收客户端遥测数据。
- 采用换行符作为消息结束标志。
- 校验遥测字段和数值合法性。
- 根据消息序号拒绝重复或过期的数据。
- 使用多线程处理不同客户端连接。
- 使用互斥锁保护共享设备数据和 CSV 更新。
- 当在线设备温度超过 80℃ 时输出高温告警。
- 使用 CSV 保存设备状态，并支持旧三列格式的读取。
- 使用 CMake 构建，使用 CTest 执行自动化测试。
- 使用 Python 编写 TCP 服务端集成测试，检查并发请求、错误消息和数据持久化。

## 3. 技术栈

- C++17
- Linux Socket API
- TCP/IP
- `std::thread`
- `std::mutex`、`std::lock_guard`
- CMake
- GCC
- CTest
- Python 3
- Git

## 4. 项目结构

```text
telemetry-platform/
├── Device.h
├── DeviceManager.h
├── DeviceManager.cpp
├── CsvStorage.h
├── CsvStorage.cpp
├── main.cpp
├── devices.csv
├── experiments/
│   ├── tcp_server.cpp
│   ├── tcp_client.cpp
│   └── thread_demo.cpp
├── tests/
│   ├── DeviceManagerTests.cpp
│   ├── TelemetryUpdateTests.cpp
│   └── TcpServerIntegration.py
├── CMakeLists.txt
└── README.md
```

## 5. 遥测消息协议

消息使用逗号分隔字段，并以换行符结束。

```text
TELEMETRY,1,36.5,1,1
```

字段定义：

| 字段 | 含义 | 示例 |
|---|---|---|
| messageType | 消息类型 | TELEMETRY |
| deviceId | 设备编号 | 1 |
| temperature | 温度 | 36.5 |
| online | 在线状态，1 在线、0 离线 | 1 |
| sequence | 消息序号 | 1 |

服务器只接受已登记设备的有效新消息。对于相同设备，消息序号必须大于当前保存的序号。

## 6. 编译项目

在 Ubuntu 环境中执行：

```bash
cmake -S . -B build
cmake --build build
```

## 7. 运行自动化测试

执行全部测试：

```bash
ctest --test-dir build --output-on-failure
```

单独运行设备更新测试：

```bash
./build/telemetry_update_tests
```

单独运行 TCP 集成测试：

```bash
ctest --test-dir build -R TcpServerIntegration -V
```

## 8. 运行 TCP 服务端和客户端

先启动服务端：

```bash
./build/tcp_server
```

再打开另一个 Ubuntu 终端，执行：

```bash
./build/tcp_client
```

服务端默认监听 `127.0.0.1:5000`。

客户端收到有效遥测消息后，服务器会根据设备编号更新状态并保存数据。若设备在线且温度超过 80℃，服务器会输出高温告警。

## 9. 当前设计限制

- 当前采用一连接一线程模型，尚未使用线程池。
- 每个连接处理一条以换行符结束的遥测消息。
- 服务端默认绑定本机回环地址，适用于本机实验。
- 设备数据使用 CSV 持久化，尚未使用数据库。
- 当前温度告警使用固定阈值 80℃。

这些限制是后续可继续改进的方向。
