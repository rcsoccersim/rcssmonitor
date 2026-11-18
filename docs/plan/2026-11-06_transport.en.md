# rcssmonitor TCP / WebSocket support: implementation plan

| Item | Content |
|---|---|
| Created | 2026-11-06 |
| Target | rcssmonitor master (ae7668a, 19.0.1) |
| Server referred to | `rcssserver-19.x-asio/` (a working tree of the asio version of rcssserver 19.x) |
| Status | Steps 1 to 4, and choosing the transport in the "Connect to ..." dialog, done (2026-11-06). Merging into develop under way from 2026-11-18 (§10) |
| Japanese version | [2026-11-06_transport.md](2026-11-06_transport.md) (the same content) |

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
- **Keep it buildable with Qt 5.12.** The README also has build steps for Ubuntu 20.04 (Qt 5.12), so `errorOccurred` (Qt 5.15 or later) cannot be used. Errors are notified by `error( QAbstractSocket::SocketError )`, connected by the `SIGNAL` / `SLOT` macros as the existing code does. → 2026-11-18: develop's minimum is now Qt 5.15 (D2 of the plan of the 3D view). Qt6 has no signal `QAbstractSocket::error()`, and connecting it by its name does not work (a TCP failure goes unnoticed), so `errorOccurred()` is connected by the function (§10).
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

→ 2026-11-18: as develop's CMake does, look for `Qt${QT_VERSION_MAJOR}::WebSockets` (Qt5 or Qt6), and add the new sources to the library `rcssmonitor_core` (§10).

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

→ 2026-11-18: the qmake build was removed in develop (D11 of the plan of the 3D view). `src/src.pro` is deleted (§10).

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

The project has no test infrastructure, so add a small test executable using Qt Test to CMake only (`enable_testing()` and `add_test()`). → 2026-11-18: develop now has its test infrastructure, so it is added with `rcssmonitor_add_test`. A test of the TCP and WebSocket transports (connecting to local servers) was added too (§10).

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
- **Moving to Qt 6** → done in develop. With the merge, the files added in this branch were ported to Qt6 too (§10).
- **Compression:** cannot be used over TCP / WebSocket, on the server side either.
- **Choosing the transport in the "Connect to..." dialog:** added depending on the conclusion of open question 1. → made on 2026-11-06 (45bca98). The option stays A.
- **CI (building and releasing the AppImage automatically):** `.github/workflows/main.yml` and `utils/appimage/`, which only master has, are to be deleted from master too, so they are not handled. They are not brought into this branch.

## 10. Record

At each step, append the date, the decisions, what was built, the bugs found and their causes, and the tests.

### 2026-11-06: Steps 1 to 4 (`feature/support-tcp-ws`)

Built in 5 commits on `feature/support-tcp-ws`, made from develop (01f880a, 19.0.1): MonitorTransport and the UDP transport (af2e1af), the TCP transport and the transport options (a41d8d9), the WebSocket transport (6759e11), the documents (78f1c15), and choosing the transport in the "Connect to ..." dialog (45bca98; out of scope in §9, but made). The unit test is of `MessageSplitter` (`test/test_message_splitter.cpp`).

### 2026-11-18: Merging into develop (`feature/support-tcp-ws`)

**Decided (user)**
- As the TCP / WebSocket support is likely to be adopted officially, it is merged into develop before Phase 10 of the plan of the 3D view. The TCP / WebSocket support of rcssserver is in its develop too (8f1097c).
- The work is done directly on `feature/support-tcp-ws`. develop is merged into this branch (no rebase, as it has been pushed), and the user makes the PR to develop (§3.5 of the plan of the 3D view).
- This plan goes into the repository. There were two copies outside it (`TRANSPORT_PLAN.md` at the top, 2026-11-06 22:30, and `docs/TRANSPORT_PLAN.md`, 12:08). The one at the top is newer: the AppImage CI items taken out, and why they are not handled added to §9. The branch's commits have no change of the CI either, so it matches the implementation. It was put in, and an English version was made.
- The plans go in `docs/plan/`, their file names in the form `<the plan's creation date>_<a word for the plan>.md`, the English version `.en.md`. The date is written `YYYY-MM-DD` (Claude). This plan is `2026-11-06_transport.md`. The plan of the 3D view is to take the same form later.

**Merging develop**
- develop (2a88791) has Phases 0 to 3 (the CI and the test infrastructure, the bug fixes, the improved JSON parser, the Qt6 support, the removal of qmake). The 7 conflicts were resolved as follows.
  - `CMakeLists.txt`: develop's form (the desktop file, Test of Qt5 or Qt6). WebSockets is looked for by `Qt${QT_VERSION_MAJOR}`.
  - `src/CMakeLists.txt`: the new sources added to `rcssmonitor_core`, and `ws_monitor_transport.cpp` and `Qt${QT_VERSION_MAJOR}::WebSockets` to `rcssmonitor_core` too (with WebSockets).
  - `test/CMakeLists.txt`: develop's form, with `rcssmonitor_add_test(message_splitter rcssmonitor_core)`.
  - `src/Makefile.am`: both sides' sources and headers. `src/src.pro`: deleted (qmake removed). `src/main_window.cpp`: both includes.
  - `README.md`: develop's steps (CMake first, Qt6 and Qt5 on Ubuntu 22.04 / 24.04 / 26.04), with the packages of Qt WebSockets (`qt6-websockets-dev`, `libqt5websockets5-dev`), and `ENABLE_WEBSOCKET` in the list of CMake's options.
- It built with Qt6 and Qt5 (`-Werror=deprecated-declarations`), and the 14 tests passed.

**A bug found and fixed**
- On Qt6, a failure of a TCP connection (no server, for example) went unnoticed. The TCP transport connected the signal of an error by its name (`SIGNAL( error( QAbstractSocket::SocketError ) )`). Qt6 has no such signal (Qt 5.15 renamed it `errorOccurred()`). Connected by the name, it builds, and at run time Qt only prints `QObject::connect: No such signal QTcpSocket::error( QAbstractSocket::SocketError )`, and the error never comes (the liveness timer cuts it off after 5 seconds). `errorOccurred()` is connected by the function (a missing signal stops the build). The WebSocket transport also connects QWebSocket's signal of an error by the function (`errorOccurred()` from Qt 6.5, `error()` before).
- A new test `monitor_transport`, connecting to a local `QTcpServer` and `QWebSocketServer`. TCP: a NUL at the end of a message sent; what is received split at the NULs (the empty ones dropped, one that comes in pieces); the server closing told once; closing here tells nothing, and the last message reaches the server; no connection (no server) told with the reason. WebSocket: the request path `/monitor`, no Origin header, a text message without a NUL, what is received, the server closing told once, no connection told with the reason. In a build without WebSockets, the WebSocket transport is not made, with the reason. Qt's warning of a signal or a slot connected by a name that does not exist fails the test too. With the TCP error connected by its name again, it was checked to fail on Qt6 (on Qt5, which has the signal, it passes).

**CI**: the three CMake jobs (Qt5, Qt6, and Qt 6.2 of Ubuntu 22.04) install Qt WebSockets, and check that `config.h` has `HAVE_QT_WEBSOCKETS`. The autotools job builds without WebSockets, as without libdeflate, so that the form without WebSocket is compiled too. The number of parallel builds is `$(nproc)` (as in the branch of the 3D view: without a number, a Makefile build runs without a limit, and the runner may run out of memory).

**Tests**
- With Qt 6.4.2 and Qt 5.15.13 (`-Werror=deprecated-declarations`), the build and the 15 tests pass. Qt6 without WebSockets (`-DENABLE_WEBSOCKET=OFF`) passes too. autotools (Qt5) builds both with WebSockets and with `--disable-websocket`.
- rcssserver's develop (8f1097c) was built in the scratchpad, and run with a temporary `RCSS_CONF_DIR` and ports other than the usual ones (UDP 16000, TCP 16100, WebSocket 16180). With a program using `MonitorClient`, over each of UDP, TCP and WebSocket, with versions -1 (JSON) and 5, on both Qt6 and Qt5, it connected, frames came (15 in 1.5 seconds), the kick-off command changed the play mode, and the disconnect command was sent. With version 2 over TCP, it refuses with the reason before connecting. The check on screen (2D and 3D) is the user's.

**A failure of the CI**: the 5 commits (4c0b384 to 3bafa02) were pushed. Only the job of Ubuntu 22.04 (Qt 6.2) failed, installing the packages: `Unable to locate package qt6-websockets-dev`. On 22.04, Qt6 WebSockets' development package is `libqt6websockets6-dev` (6.2.4); `qt6-websockets-dev` is its name from 24.04 (checked on packages.ubuntu.com). That job of the CI and the README's steps (with 22.04's name added) were fixed.
