# rcssmonitor TCP / WebSocket support: implementation plan

| Item | Content |
|---|---|
| Created | 2026-11-06 |
| Target | rcssmonitor master (ae7668a, 19.0.1) |
| Server referred to | `rcssserver-19.x-asio/` (a working tree of the asio version of rcssserver 19.x) |
| Status | Plan only. No code changed yet |

## 1. Aim and approach

- Make rcssmonitor support the TCP / WebSocket connections that the asio version of rcssserver (the "new server" below) provides.
- Abstract the channel as a "transport that sends and receives one message at a time", and switch between the UDP / TCP / WebSocket implementations by an option. This is the same idea as `rcss::Connection` in the new server.
- UDP stays the default, and nothing changes unless an option is given.
- WebSocket uses the Qt WebSockets module, an optional dependency at build time. Without it, the UDP / TCP version can still be built.

Ideas not taken:

- **Boost.Beast**: it needs an `io_context` running apart from Qt's event loop.
- **A WebSocket client of our own**: the handshake, masking, fragmented frames and ping/pong would have to be maintained by ourselves.

## 2. Premise: the new server's specification

Sources referred to:

- `rcssserver-19.x-asio/src/transport/` (`connection.h`, `tcpconnection.cpp`, `wsconnection.cpp`, `wsserver.cpp`, `pendingconnections.cpp`)
- `rcssserver-19.x-asio/src/stadium.cpp` (`openAcceptors()`, `openWSServer()`, `parseMonitorInit()`)
- `rcssserver-19.x-asio/src/serverparam.cpp` (`tcp_port`, `ws_port`, `ws_allowed_origins`)

### 2.1 Ports and paths

| Transport | Server parameter | Default | Where the monitor connects |
|---|---|---|---|
| UDP | `server::port` | 6000 | The port shared with the players |
| TCP | `server::tcp_port` | 0 (disabled) | The port shared with the players |
| WebSocket | `server::ws_port` | 0 (disabled) | The path `/monitor` (for monitors only) or `/player` |

- WebSocket shares one port among all the services, and sorts them by the request path of the handshake (`/player`, `/monitor`, `/coach`, `/olcoach`). A query string is ignored.
- `server::ws_allowed_origins` is checked only when there is an Origin header. A client without the header is always accepted.
- TCP / WebSocket listen dual-stack on IPv6, or on IPv4 if that is not possible.

### 2.2 Message delimiters

| Transport | Server → monitor | Monitor → server |
|---|---|---|
| UDP | One datagram | One datagram |
| TCP | Always ends with `\0` | Delimited by `\0` or a newline (`\r\n` too) |
| WebSocket | One text message, without `\0` | One message. A trailing `\0`, newline or `\r` is removed |

- A message from the monitor to the server must be shorter than `MaxMesg` (8192 bytes). A longer one closes the connection.
- A message from the server to the monitor has no length limit. If more than 1 MiB waits to be sent, the server closes that client.

### 2.3 Constraints on the monitor connection

- **TCP / WebSocket cannot carry binary data** (`Connection::isBinarySafe()` is false). `(dispinit version 1)` and `(dispinit version 2)` are refused with `(error illegal_client_version)`. Compression cannot be used either.
- **dispinit is needed within 10 seconds of connecting** (`PendingConnections::TIMEOUT`). Past that, the connection is closed. Up to 64 connections wait to be accepted.
- **Over UDP, the port that replies changes** (the replies come from a port for the monitor only). Over TCP / WebSocket, the connection itself is dedicated, so it does not change.
- On `(dispbye)`, the server disables the monitor and closes the connection.
- Beyond `server::max_monitors`, `(error no_more_monitor)` is returned in reply to dispinit.
- After connecting too, an illegal command gets `(error illegal_command_form)` and the like. This is not a connection failure.

## 3. The present rcssmonitor and its problems

| Where | Now | Problem |
|---|---|---|
| `MonitorClient` | Holds a `QUdpSocket` directly | The transport cannot be replaced |
| Receiving | Four loops of the same shape, one for each version | Following the replying port is also duplicated four times |
| Starting a connection | Resolves the name synchronously and does `bind(0)` in the constructor, and tells success by `isConnected()` | Over TCP / WebSocket, the result of connecting comes asynchronously |
| dispinit | `MainWindow` sends it right after creating the client | It is sent before the connection is established |
| Liveness | `disconnectRequested` after 5 seconds without traffic, `reconnectRequested` in the auto-reconnect mode | No way to detect the socket being closed |
| Deletion | `MainWindow` `delete`s the client while handling `disconnectRequested` | The client is deleted inside its own slot |
| Receive buffer | Reads into `char buf[8192]` and sets `buf[n] = '\0'` | Writes out of the array when n is 8192 (an existing bug) |

## 4. Design

### 4.1 Structure

```
MainWindow
  └── MonitorClient            protocol layer: dispinit, building commands, dispatching what is received, liveness
        └── MonitorTransport   abstract class: sending and receiving one message at a time
              ├── UdpMonitorTransport   QUdpSocket
              ├── TcpMonitorTransport   QTcpSocket + MessageSplitter
              └── WsMonitorTransport    QWebSocket (only with HAVE_QT_WEBSOCKETS)
```

### 4.2 MonitorTransport

```cpp
class MonitorTransport
    : public QObject {

    Q_OBJECT

public:
    enum Type { UDP, TCP, WebSocket };

    explicit MonitorTransport( QObject * parent );
    ~MonitorTransport() override;

    virtual Type type() const = 0;

    //! start connecting. returns false only for a failure known at once, such as a failed name lookup
    virtual bool open( const QString & host, quint16 port, QString * error ) = 0;

    //! close the connection. the data waiting to be sent is sent out as far as possible
    virtual void close() = 0;

    //! true after connected(), until disconnected
    virtual bool isOpen() const = 0;

    //! whether the binary format of v1/v2 can be carried
    virtual bool isBinarySafe() const = 0;

    //! send one message. the implementation adds the delimiter
    virtual bool sendMessage( const QByteArray & msg ) = 0;

    //! take out one received message. returns false if there is none
    virtual bool popMessage( QByteArray & msg ) = 0;

    //! the peer's notation for logs and the status bar, e.g. "tcp://127.0.0.1:6000"
    virtual QString peerName() const = 0;

signals:
    void connected();
    void messagesAvailable();
    void disconnected( const QString & reason );
};

//! returns nullptr on failure, with the reason in error
MonitorTransport * create_monitor_transport( const MonitorTransport::Type type,
                                             const QString & ws_path,
                                             QObject * parent,
                                             QString * error );
```

Decisions:

- **Receiving is "notified by a signal, taken out by `popMessage()`".** Even when one read brings several messages, `MonitorClient` can emit `received()` only once. The number of redraws stays as it is now.
- **`connected()` is emitted only once, for every transport.** UDP emits it by a 0 ms timer after `open()` succeeds, so that the flow is the same as the other transports'.
- **`disconnected()` is emitted only once too.** An internal flag prevents duplicates. UDP has no notion of connection, so it does not emit it.
- **A received message can be used as a NUL-terminated string by `QByteArray::constData()`.** It can be given to `DispHolder::addJSON()` and others as it is.
- **Keep it buildable with Qt 5.12.** The README also has build steps for Ubuntu 20.04 (Qt 5.12), so `errorOccurred` (Qt 5.15 or later) cannot be used. Errors are notified by `error( QAbstractSocket::SocketError )`, connected by the `SIGNAL` / `SLOT` macros as the existing code does.
- **Ownership is managed by QObject's parent-child relation.** The transport's parent is `MonitorClient`.

### 4.3 The transports' implementations

| | UDP | TCP | WebSocket |
|---|---|---|---|
| Qt class | `QUdpSocket` | `QTcpSocket` | `QWebSocket` |
| open | Synchronous name lookup and `bind(0)` | `connectToHost()` (asynchronous) | `open( ws://host:port/path )` (asynchronous) |
| Send | `writeDatagram()` including the trailing `\0` | `write()` with `\0` appended | `sendTextMessage()` without the `\0` |
| Receive | Allocate by `pendingDatagramSize()` and read one at a time | Split at each `\0` by `MessageSplitter` | `textMessageReceived` and `binaryMessageReceived` |
| Detecting disconnection | None (left to the liveness timer) | `disconnected` and `error` | `disconnected` and `error` |
| `isBinarySafe()` | true | false | false |

**UDP**

- Move the UDP handling of the present `MonitorClient`. Following the replying port is also closed into this class.
- Allocate the receive buffer by `pendingDatagramSize()`, removing the existing out-of-array write.

**TCP**

- On `connected`, set `QAbstractSocket::LowDelayOption` to 1. The server sets `no_delay` too.
- `MessageSplitter` is a small class that only stores the received bytes and cuts them out at each `\0`. It does not depend on Qt's sockets, so that it can be unit tested.
- Delimit by `\0` only, not by newlines, since the server always terminates with `\0`. Empty messages are dropped.
- If an incomplete message exceeds 1 MiB, emit `disconnected( "too long message" )` and close.
- `close()` calls `flush()` and then `disconnectFromHost()`, so that `(dispbye)` is sent out even if `deleteLater()` follows right after.

**WebSocket**

- Build the URL with `QUrl` (scheme `ws`, host, port, path). `QUrl` also handles the brackets of IPv6 addresses.
- When the server refuses with 404 (a wrong path) or 403 (the Origin refused), `error` comes. Give `errorString()` to `disconnected()` as the reason.
- No origin is given. `QWebSocket` sends no Origin header without an origin, so it is not affected by `server::ws_allowed_origins`. This behavior is checked by test item 10.

### 4.4 MonitorClient

The functions that build commands (`sendKickOff()` and others) stay as they are; only where `sendCommand()` sends to is replaced by the transport.

```cpp
MonitorClient( QObject * parent,
               DispHolder & disp_holder,
               MonitorTransport * transport, // the ownership moves to MonitorClient
               const int version );

//! start connecting. returns false only for a failure known at once
bool start( const QString & host, quint16 port, QString * error );

signals:
    void received();
    void disconnectRequested();
    void reconnectRequested();
    void errorMessage( const QString & msg ); // new, for the status bar
```

- **Starting:** `start()` checks the combination of the protocol version and the transport before calling `open()`. If v1 / v2 is given over TCP / WebSocket, it is an error, so that the server's text replies are not read as binary structures.
- **Liveness:** the timer starts right after `open()`. A long wait for a TCP connection can also be cut off by this timer. What happens on timeout is as now.
- **dispinit:** sent after `connected()` comes. The direct call from `MainWindow` is removed.
- **Receiving:** on `messagesAvailable()`, take out all the messages by `popMessage()` and give them one by one to the dispatch function. If there is at least one, reset the timer and emit `received()` once.
- **Dispatching:**
  - When the first message after connecting starts with `(error`, the connection failed: emit `errorMessage()` and ask to disconnect. Later `(error ...)` messages are only logged, as before.
  - JSON (version -1) goes to `addJSON()`.
  - v3 or later goes to `addDispInfoV3()`.
  - v2 / v1: copy `min( size, sizeof )` bytes into a zero-initialized structure and give it to `addDispInfoV2()` / `addDispInfoV1()`.
- **Disconnection:** on the transport's `disconnected()`, tell the reason by `errorMessage()`. Then emit `reconnectRequested()` in the auto-reconnect mode, or `disconnectRequested()` otherwise.
- **Ending:** `disconnect()` stops the timer and, if connected, sends `(dispbye)` and then calls `close()`. The server detects the disconnection even if dispbye does not arrive, so it does not matter if it cannot be sent.
- **isConnected():** returns `transport->isOpen()`.

### 4.5 MainWindow

`connectMonitorTo()`:

1. Get the transport type from `Options`, and make the transport by `create_monitor_transport()`. If that fails, as in a build without WebSocket, show the reason in the status bar and stop.
2. Make the `MonitorClient`, connect its signals, and then call `start()`. If it returns false, show the reason and delete the client.
3. Clearing the data, enabling the actions and switching to the live mode are done as now. The direct call of `sendDispInit()` is removed.
4. The "Connect to ..." text uses `peerName()`, so that the transport can be seen.

`disconnectMonitor()`:

- Replace `delete` by `deleteLater()`.
- Keep the signals connected directly. With queued connections, an old client's notice could arrive after disconnecting and close a new connection.

Add one new slot, connecting `errorMessage()` to showing it in the status bar.

### 4.6 Options

| Setting | Command line | QSettings key | Default |
|---|---|---|---|
| Transport | `--server-transport udp\|tcp\|ws` | `server_transport` | `udp` |
| WebSocket path | `--server-ws-path PATH` | `server_ws_path` | `/monitor` |
| Port number | `--server-port PORT` (existing) | `server_port` (existing) | 6000 |

- Add them in all three places of `options.cpp`: the `USE_Q_COMMAND_LINE_PARSER` side, the `HAVE_BOOST_PROGRAM_OPTIONS` side, and reading and writing the QSettings.
- The transport names are case insensitive. `websocket` is also accepted as another name of `ws`. An illegal value is an error when the command line is parsed.
- The port number is common to all transports. The new server's TCP / WebSocket ports have no default, so no default per transport is set.

Examples:

```sh
rcssserver server::tcp_port=6000 server::ws_port=6080

rcssmonitor --connect                                              # UDP (as before)
rcssmonitor --connect --server-transport tcp --server-port 6000   # TCP
rcssmonitor --connect --server-transport ws  --server-port 6080   # WebSocket
```

### 4.7 Build systems

**CMake**

The top-level `CMakeLists.txt`:

```cmake
option(ENABLE_WEBSOCKET "Enable the WebSocket connection (requires Qt5 WebSockets)" ON)
if(ENABLE_WEBSOCKET)
  find_package(Qt5 COMPONENTS WebSockets QUIET)
  if(Qt5WebSockets_FOUND)
    set(HAVE_QT_WEBSOCKETS TRUE)
  else()
    message(WARNING "Qt5 WebSockets not found. The WebSocket connection is disabled.")
  endif()
endif()
```

`cmake-config.h.in`:

```c
#cmakedefine HAVE_QT_WEBSOCKETS
```

`src/CMakeLists.txt`: add the new sources to `add_executable`, and add the WebSocket implementation conditionally. `CMAKE_AUTOMOC` handles moc.

```cmake
if(HAVE_QT_WEBSOCKETS)
  target_sources(rcssmonitor PRIVATE ws_monitor_transport.cpp)
  target_link_libraries(rcssmonitor PRIVATE Qt5::WebSockets)
endif()
```

**autotools**

- `m4/ax_have_qt.m4`: this macro links all the modules available to qmake, but websockets is not in its list. Add `qtHaveModule(websockets): QT += websockets`.
- `configure.ac`:

```m4
AC_ARG_ENABLE([websocket],
  [AS_HELP_STRING([--disable-websocket], [disable the WebSocket connection])],
  [], [enable_websocket=yes])
have_qt_websockets=no
if test "x$enable_websocket" = "xyes"; then
  AC_LANG_PUSH([C++])
  save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $QT_CXXFLAGS"
  AC_CHECK_HEADER([QWebSocket], [have_qt_websockets=yes])
  CPPFLAGS="$save_CPPFLAGS"
  AC_LANG_POP([C++])
fi
if test "x$have_qt_websockets" = "xyes"; then
  AC_DEFINE([HAVE_QT_WEBSOCKETS], [1], [Define to 1 if Qt WebSockets is available.])
fi
AM_CONDITIONAL([HAVE_QT_WEBSOCKETS], [test "x$have_qt_websockets" = "xyes"])
```

- `src/Makefile.am`: add the new sources, headers and moc files to the lists. Add the WebSocket implementation conditionally.

```make
if HAVE_QT_WEBSOCKETS
rcssmonitor_SOURCES += ws_monitor_transport.cpp
nodist_rcssmonitor_SOURCES += moc_ws_monitor_transport.cpp
endif
```

**qmake**

`src/src.pro`:

```qmake
qtHaveModule(websockets) {
  QT += websockets
  DEFINES += HAVE_QT_WEBSOCKETS
  HEADERS += ws_monitor_transport.h
  SOURCES += ws_monitor_transport.cpp
}
```

**Notes on the development environment**

- Qt WebSockets is not installed on the present development machine. `sudo apt install libqt5websockets5-dev` is needed.
- anaconda's qmake (Qt 5.15.2) is at the head of PATH, and autotools uses it. Make the system's qmake found first.
- CMake may also find anaconda's Qt. If needed, give `-DQt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5`.

## 5. Files to change

New:

| File | Content |
|---|---|
| `src/monitor_transport.h`, `.cpp` | The abstract class and the factory function |
| `src/udp_monitor_transport.h`, `.cpp` | The UDP implementation, moved from `MonitorClient` |
| `src/tcp_monitor_transport.h`, `.cpp` | The TCP implementation |
| `src/message_splitter.h`, `.cpp` | Splitting at `\0` |
| `src/ws_monitor_transport.h`, `.cpp` | The WebSocket implementation, built conditionally |

Changed:

| File | Content |
|---|---|
| `src/monitor_client.h`, `.cpp` | Using the transport, unifying the receiving, the flow of connecting |
| `src/main_window.h`, `.cpp` | Connecting asynchronously, `deleteLater()`, showing errors |
| `src/options.h`, `.cpp` | The new options |
| `CMakeLists.txt`, `src/CMakeLists.txt`, `cmake-config.h.in` | CMake |
| `configure.ac`, `m4/ax_have_qt.m4`, `src/Makefile.am` | autotools |
| `src/src.pro` | qmake |
| `README.md`, `NEWS` | Usage and the change history |

## 6. Implementation steps

In an order that keeps a working state at each step and can be committed step by step.

### Step 1: abstract, still with UDP

- Add `monitor_transport` and `udp_monitor_transport`.
- Rewrite `MonitorClient` to go through the transport, and unify the receiving loops into one. Fix the out-of-array write here too.
- Add `start()`, dispinit on `connected()`, `deleteLater()`, and showing `errorMessage()`.
- Add the new files to the three build systems.

Done when: connecting to the existing server over UDP, the display of JSON, v3, v4 and v5, sending commands, auto-reconnect and auto-quit work as before.

### Step 2: add TCP and the transport options

- Add `message_splitter` and `tcp_monitor_transport`.
- Add `--server-transport` and `--server-ws-path`. At this step, `ws` is an error.
- Add the check of v1 / v2 over TCP / WebSocket.

Done when: with the new server started with `server::tcp_port=6000`, the items of chapter 7 pass over TCP. The unit test of `MessageSplitter` passes.

### Step 3: add WebSocket

- Add `ws_monitor_transport`.
- Add the detection of Qt WebSockets to the three build systems.

Done when: the build passes both with and without Qt WebSockets. With it, the items of chapter 7 pass over WebSocket. Without it, `--server-transport ws` shows an error.

### Step 4: update the documents

- Write the usage and an example of the server's settings in the README. Update NEWS and the `--help` text.

Done when: following the README, one can connect to the new server over TCP and WebSocket.

## 7. Test plan

### 7.1 Items to check

| # | Item | UDP | TCP | WebSocket |
|---|---|---|---|---|
| 1 | A game is shown with JSON, v3, v4 and v5 | ○ | ○ | ○ |
| 2 | What happens when v1 / v2 is given | As before | An error before connecting | An error before connecting |
| 3 | Kick-off, drop ball, free kick, cards, moving by the trainer and changing the play mode reach the server | ○ | ○ | ○ |
| 4 | Without the server running, an error is shown normally, and retries go on in the auto-reconnect mode | ○ | ○ | ○ |
| 5 | Stopping the server during a game is taken as a disconnection | After 5 seconds | At once | At once |
| 6 | After the disconnect operation, the server's log shows the monitor leaving | ○ | ○ | ○ |
| 7 | Beyond `server::max_monitors`, an error is shown | ○ | ○ | ○ |
| 8 | `(error illegal_command_form)` for an illegal trainer command does not disconnect | ○ | ○ | ○ |
| 9 | It connects with `--server-ws-path /player` too, and a path that does not exist is an error | - | - | ○ |
| 10 | It connects to a server with `server::ws_allowed_origins` set | - | - | ○ |
| 11 | The auto-quit mode works as before | ○ | ○ | ○ |
| 12 | Playing a log file is not affected | ○ | - | - |

### 7.2 Unit tests

For `MessageSplitter`, check these cases:

- One message comes in several reads.
- One read holds several messages.
- With consecutive `\0`s, the empty messages are dropped.
- An incomplete message beyond the limit is an error.

The project has no test infrastructure, so add a small test executable using Qt Test to CMake only (`enable_testing()` and `add_test()`).

### 7.3 Checking what is sent

The bytes that the monitor sends over TCP can be checked by receiving them with nc in place of the server. It is right if `(dispinit version 5)` is followed by `00`.

```sh
nc -l 6000 | xxd
rcssmonitor --connect --server-transport tcp --server-port 6000
```

## 8. Open questions

| # | Item | Choices | Recommended |
|---|---|---|---|
| 1 | The form of the option | A: add `--server-transport` (this plan). B: a URL form such as `--server tcp://host:6000` | A. It keeps the compatibility with the existing `--server-host` / `--server-port` and the settings file. A URL form can be accepted later in the input field of the "Connect to..." dialog |
| 2 | The timeout without traffic over TCP / WebSocket | Keep / remove | Keep. A disconnection can be seen by the socket, but a frozen server can be noticed only by the timeout |
| 3 | The default port number | 6000 common to all transports / a value per transport | Common. The new server's TCP / WebSocket ports have no default |

## 9. Out of scope

- **wss:// (TLS):** the new server itself does not support TLS. Use through a reverse proxy is a future task.
- **Moving to Qt 6**
- **Compression:** cannot be used over TCP / WebSocket, on the server side either.
- **Choosing the transport in the "Connect to..." dialog:** added depending on the conclusion of open question 1.
- **CI (building and releasing the AppImage automatically):** `.github/workflows/main.yml` and `utils/appimage/`, which only master has, are to be deleted from master too, so they are not handled. They are not brought into this branch.
