# Telemetry Platform

## Linux C++ 多设备遥测与故障诊断平台

### 1. 项目简介

本项目使用 C++17 和 Linux Socket 开发一个多设备遥测管理平台，模拟传感器、机器人及其他智能设备向服务器上传运行状态的场景。

服务端负责设备注册、心跳检测、遥测数据接收、异常处理和数据持久化，并支持多个 TCP 客户端同时接入。

项目重点是 Linux C++ 网络编程、协议设计、多线程同步、设备状态管理与软件测试。

### 2. 技术栈

- 编程语言：C++17、Python 3
- 开发环境：Linux / Ubuntu WSL
- 构建工具：CMake
- 网络通信：TCP Socket
- 并发处理：std::thread、std::mutex
- 运行计数：std::atomic
- 数据存储：CSV
- 调试工具：GDB
- 测试工具：CTest、Python 集成测试
- 版本管理：Git

### 3. 核心功能

**设备管理**
- 设备注册与重复注册检查
- 设备在线状态管理
- 温度数据更新和高温报警
- 设备状态 CSV 持久化

**网络通信**
- TCP 服务端接入
- 多客户端并发处理
- TCP 长连接
- 半包与粘包处理
- 消息长度限制
- 接收超时和异常断连处理

**通信协议**
- REGISTER：设备注册
- HEARTBEAT：心跳上报
- TELEMETRY：遥测上报
- ACK / ERROR：服务器响应
- 消息序列号校验
- 非法消息和重复序列号拒绝

**故障诊断**
- 心跳超时自动离线
- 新心跳恢复在线状态
- 高温报警
- 服务器消息与错误统计

### 4. 系统架构

```text
Python / C++ Device Clients
             |
             | TCP
             v
       TCP Server
             |
       Client Worker
             |
       Message Framing
             |
       Protocol Parser
             |
       Device Manager
             |
       CSV Storage

Background Heartbeat Monitor
             |
       Timeout Detection
             |
       Device State Update
```

服务端使用一个客户端连接对应一个工作线程的方式处理网络请求，并通过互斥锁保护共享设备数据。

### 5. 通信协议示例

设备注册：

```text
REGISTER,100
```

心跳消息：

```text
HEARTBEAT,100,1
```

遥测消息：

```text
TELEMETRY,100,36.5,1,2
```

遥测字段依次表示：消息类型、设备 ID、温度、在线状态、递增序列号。

每条消息以换行符 `\n` 结束。

### 6. 编译与运行

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build
```

启动服务端：

```bash
./build/tcp_server
```

启动命令行设备管理程序：

```bash
./build/telemetry
```

服务端默认监听：

```text
127.0.0.1:5000
```

### 7. 自动化测试

执行：

```bash
ctest --test-dir build --output-on-failure
```

目前包括：

- DeviceManagerTests
- TelemetryUpdateTests
- ProtocolMessageTests
- TcpServerIntegration

另外可运行心跳超时测试：

```bash
python3 tests/HeartbeatTimeoutIntegration.py ./build/tcp_server
```

注意：TCP 集成测试会自行启动服务端，运行前应确保 5000 端口未被其他服务端占用。

### 8. 性能测试

性能测试脚本：

```bash
python3 scripts/stress_test.py \
    --clients 5 \
    --duration 10 \
    --rate 2
```

正式测试示例：

```bash
python3 scripts/stress_test.py \
    --clients 20 \
    --duration 30 \
    --rate 5
```

性能指标包括：

- 成功处理消息数
- 请求失败数
- 吞吐量
- 平均响应延迟
- P95 响应延迟

具体测试环境和实测结果见：

`docs/performance_test.md`

### 9. 已知限制

- 使用阻塞式 TCP Socket 和每连接一线程的模型，未使用 epoll 或线程池。
- 当前采用文本行分隔协议，而非二进制协议。
- 使用 CSV 持久化，未引入数据库。
- 心跳超时检测以服务端运行期间维护的心跳时间为基础。
- 未声明经过验证的大规模高并发性能。

### 10. 项目总结

通过本项目，实践了 Linux C++ Socket 编程、C++ 多线程同步、协议解析、设备状态管理、CSV 持久化、自动化测试与基础性能测量，形成了从设备消息上报到状态更新、异常诊断和持久化的完整处理链路。
