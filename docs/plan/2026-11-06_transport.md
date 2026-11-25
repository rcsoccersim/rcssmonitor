# rcssmonitor の TCP / WebSocket 対応 実装計画

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-11-06 |
| 最終更新日 | 2026-11-24 |
| 対象 | rcssmonitor master (ae7668a, 19.0.1) |
| ブランチ | `feature/support-tcp-ws` (develop 01f880a から) |
| 状態 | 完了。PR #68 (Merge Commit) で develop に取り込んだ (a6df084、2026-11-18)。段階ごとの進み具合は、次の「進み具合」の表 |
| 英語版 | [2026-11-06_transport_en.md](2026-11-06_transport_en.md) (内容は同じ) |
| 参照したサーバ | `rcssserver-19.x-asio/` (asio 版 rcssserver 19.x の作業ツリー) |

## 進み具合

段階ごとの作業と完了条件は §6、経緯は §10 にある。段階ごとのユーザの確認と CI は記録にない (CI は、3D 表示の計画の Phase 0 で後から作った)。取り込みの前に、ユーザが UDP、TCP、WebSocket のライブ接続を確かめた。

| 段階 | 内容 | 状態 | ユーザの確認 | CI | コミット | プッシュ |
|---|---|---|---|---|---|---|
| 1 | UDP のまま抽象化する (§6 段階 1) | 完了 | — | — | af2e1af | 済み |
| 2 | TCP と接続方式のオプションを追加する (§6 段階 2) | 完了 | — | — | a41d8d9 | 済み |
| 3 | WebSocket を追加する (§6 段階 3) | 完了 | — | — | 6759e11 | 済み |
| 4 | 文書を更新する (§6 段階 4) | 完了 | — | — | 78f1c15 | 済み |
| — | 「Connect to ...」ダイアログでの方式の選択 (§9 では対象外としていた) | 完了 | — | — | 45bca98 | 済み |
| — | develop のマージ、Qt6 の通信の誤りの修正、テスト、CI (§10 の 2026-11-18) | 完了 | 済み | 通過 | 4c0b384〜35e0957 | 済み |
| — | develop への取り込み (PR #68、Merge Commit) | 完了 | 済み | 通過 | a6df084 | 済み |

## 1. 目的と方針

- asio 版 rcssserver (以下「新サーバ」) が提供する TCP / WebSocket 接続に rcssmonitor を対応させる。
- 通信路を「1 メッセージ単位で送受信するトランスポート」として抽象化し、UDP / TCP / WebSocket の実装をオプションで切り替える。新サーバ側の `rcss::Connection` と同じ考え方をとる。
- 既定は従来どおり UDP とし、オプションを指定しない限り挙動を変えない。
- WebSocket には Qt WebSockets モジュールを使い、ビルド時の任意依存とする。見つからない環境でも UDP / TCP 版はビルドできる。

採らなかった案:

- **Boost.Beast**: Qt のイベントループとは別に `io_context` を回す必要がある。
- **WebSocket クライアントの自作**: ハンドシェイク、マスク処理、分割フレーム、ping/pong を自前で保守することになる。

## 2. 前提: 新サーバの仕様

参照したソース:

- `rcssserver-19.x-asio/src/transport/` (`connection.h`, `tcpconnection.cpp`, `wsconnection.cpp`, `wsserver.cpp`, `pendingconnections.cpp`)
- `rcssserver-19.x-asio/src/stadium.cpp` (`openAcceptors()`, `openWSServer()`, `parseMonitorInit()`)
- `rcssserver-19.x-asio/src/serverparam.cpp` (`tcp_port`, `ws_port`, `ws_allowed_origins`)

### 2.1 ポートとパス

| 方式 | サーバパラメータ | 既定値 | モニタの接続先 |
|---|---|---|---|
| UDP | `server::port` | 6000 | プレイヤーと共用のポート |
| TCP | `server::tcp_port` | 0 (無効) | プレイヤーと共用のポート |
| WebSocket | `server::ws_port` | 0 (無効) | パス `/monitor` (モニタ専用) または `/player` |

- WebSocket は 1 つのポートを全サービスで共用し、ハンドシェイクのリクエストパスで振り分ける (`/player`, `/monitor`, `/coach`, `/olcoach`)。クエリ文字列は無視される。
- `server::ws_allowed_origins` は、Origin ヘッダがあるときだけ検査される。ヘッダのないクライアントは常に受け付けられる。
- TCP / WebSocket は IPv6 デュアルスタックで待ち受け、できなければ IPv4 で待ち受ける。

### 2.2 メッセージの区切り

| 方式 | サーバ → モニタ | モニタ → サーバ |
|---|---|---|
| UDP | 1 データグラム | 1 データグラム |
| TCP | 末尾に必ず `\0` を付ける | `\0` または改行で区切る (`\r\n` も可) |
| WebSocket | 1 テキストメッセージ。`\0` は付けない | 1 メッセージ。末尾の `\0`・改行・`\r` は除去される |

- モニタからサーバへのメッセージは `MaxMesg` (8192 バイト) 未満でなければならない。超えると切断される。
- サーバからモニタへのメッセージ長には上限がない。送信待ちが 1 MiB を超えると、サーバはそのクライアントを切断する。

### 2.3 モニタ接続に関わる制約

- **TCP / WebSocket はバイナリを通せない** (`Connection::isBinarySafe()` が false)。`(dispinit version 1)` と `(dispinit version 2)` は `(error illegal_client_version)` で拒否される。圧縮も使えない。
- **接続後 10 秒以内に dispinit が必要** (`PendingConnections::TIMEOUT`)。過ぎると接続は閉じられる。受付待ちの接続は 64 本まで。
- **UDP では応答元ポートが変わる** (モニタ専用ポートから返信される)。TCP / WebSocket では接続そのものが専用なので変わらない。
- `(dispbye)` を受けると、サーバはモニタを無効化して接続を閉じる。
- `server::max_monitors` を超えると、dispinit への応答として `(error no_more_monitor)` が返る。
- 接続後も、不正なコマンドには `(error illegal_command_form)` などが返る。これは接続失敗ではない。

## 3. 現状の rcssmonitor と課題

| 箇所 | 現状 | 課題 |
|---|---|---|
| `MonitorClient` | `QUdpSocket` を直接保持する | 方式を差し替えられない |
| 受信処理 | バージョンごとに同じ形のループが 4 つある | 応答元ポートへの追従処理も 4 重に重複している |
| 接続開始 | コンストラクタで同期 DNS 解決と `bind(0)` を行い、`isConnected()` で成否を判定する | TCP / WebSocket では接続結果が非同期に届く |
| dispinit | `MainWindow` が生成直後に送る | 接続確立前に送ってしまう |
| 生存確認 | 無通信 5 秒で `disconnectRequested`、自動再接続モードでは `reconnectRequested` | ソケットの切断を検知する経路がない |
| 削除 | `disconnectRequested` の処理中に `MainWindow` がクライアントを `delete` する | クライアントが自分のスロットの中で削除される |
| 受信バッファ | `char buf[8192]` に読み込み、`buf[n] = '\0'` とする | n が 8192 のとき配列外に書き込む (既存バグ) |

## 4. 設計

### 4.1 構成

```
MainWindow
  └── MonitorClient            プロトコル層: dispinit、コマンド生成、受信の振り分け、生存確認
        └── MonitorTransport   抽象クラス: 1 メッセージ単位の送受信
              ├── UdpMonitorTransport   QUdpSocket
              ├── TcpMonitorTransport   QTcpSocket + MessageSplitter
              └── WsMonitorTransport    QWebSocket (HAVE_QT_WEBSOCKETS のときだけ)
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

    //! 接続を始める。DNS 解決の失敗など、その場で分かる失敗のときだけ false を返す
    virtual bool open( const QString & host, quint16 port, QString * error ) = 0;

    //! 接続を閉じる。送信待ちのデータはできるだけ送り出す
    virtual void close() = 0;

    //! connected() の後、切断されるまで true
    virtual bool isOpen() const = 0;

    //! v1/v2 のバイナリ形式を運べるか
    virtual bool isBinarySafe() const = 0;

    //! 1 メッセージを送る。区切り文字は実装が付ける
    virtual bool sendMessage( const QByteArray & msg ) = 0;

    //! 受信済みのメッセージを 1 件取り出す。なければ false を返す
    virtual bool popMessage( QByteArray & msg ) = 0;

    //! ログとステータスバー用の接続先の表記。例: "tcp://127.0.0.1:6000"
    virtual QString peerName() const = 0;

signals:
    void connected();
    void messagesAvailable();
    void disconnected( const QString & reason );
};

//! 失敗したら nullptr を返し、error に理由を入れる
MonitorTransport * create_monitor_transport( const MonitorTransport::Type type,
                                             const QString & ws_path,
                                             QObject * parent,
                                             QString * error );
```

決めごと:

- **受信は「通知はシグナル、取り出しは `popMessage()`」とする。** 1 回の読み込みで複数のメッセージが届いても、`MonitorClient` は `received()` を 1 回だけ出せる。再描画の回数は現状と変わらない。
- **`connected()` は全方式で 1 回だけ発行する。** UDP は `open()` が成功した後に 0 ms タイマで発行し、他の方式と処理の流れを揃える。
- **`disconnected()` も 1 回だけ発行する。** 内部フラグで重複を防ぐ。UDP には接続の概念がないので発行しない。
- **受け取ったメッセージは `QByteArray::constData()` で NUL 終端の文字列として扱える。** `DispHolder::addJSON()` などにそのまま渡せる。
- **Qt 5.12 でもビルドできるようにする。** README には Ubuntu 20.04 (Qt 5.12) 向けのビルド手順もあるので、`errorOccurred` (Qt 5.15 以降) は使えない。エラー通知は `error( QAbstractSocket::SocketError )` を、既存コードと同じ `SIGNAL` / `SLOT` マクロで接続する。 → 2026-11-18: develop の下限は Qt 5.15 になった (3D 表示の計画の D2)。Qt6 には `QAbstractSocket::error()` のシグナルがなく、名前でつなぐと動かない (TCP の失敗に気づかない) ので、`errorOccurred()` を関数でつなぐ (§10)。
- **所有権は QObject の親子関係で管理する。** トランスポートの親は `MonitorClient` とする。

### 4.3 トランスポートの実装

| | UDP | TCP | WebSocket |
|---|---|---|---|
| Qt のクラス | `QUdpSocket` | `QTcpSocket` | `QWebSocket` |
| open | 同期 DNS 解決と `bind(0)` | `connectToHost()` (非同期) | `open( ws://host:port/path )` (非同期) |
| 送信 | 末尾の `\0` を含めて `writeDatagram()` | 末尾に `\0` を付けて `write()` | `\0` を除いて `sendTextMessage()` |
| 受信 | `pendingDatagramSize()` で確保し 1 件ずつ読む | `MessageSplitter` で `\0` ごとに分割 | `textMessageReceived` と `binaryMessageReceived` |
| 切断の検知 | なし (生存確認タイマに任せる) | `disconnected` と `error` | `disconnected` と `error` |
| `isBinarySafe()` | true | false | false |

**UDP**

- 現在の `MonitorClient` の UDP 処理を移す。応答元ポートへの追従もこのクラスの中に閉じ込める。
- 受信バッファを `pendingDatagramSize()` で確保し、既存の配列外書き込みをなくす。

**TCP**

- `connected` を受けたら `QAbstractSocket::LowDelayOption` を 1 にする。サーバ側も `no_delay` を設定している。
- `MessageSplitter` は、受信したバイト列をためて `\0` ごとに切り出すだけの小さなクラスにする。Qt のソケットに依存させず、単体テストできるようにする。
- 区切りは `\0` だけとし、改行では区切らない。サーバは常に `\0` で終端するため。空のメッセージは捨てる。
- 未完了のメッセージが 1 MiB を超えたら、`disconnected( "too long message" )` を出して閉じる。
- `close()` では `flush()` してから `disconnectFromHost()` を呼ぶ。直後に `deleteLater()` されても `(dispbye)` が送り出されるようにするため。

**WebSocket**

- URL は `QUrl` で組み立てる (スキーム `ws`、ホスト、ポート、パス)。IPv6 アドレスの角括弧も `QUrl` が扱う。
- サーバが 404 (パス違い) や 403 (Origin 拒否) で断ると `error` が届く。`errorString()` を理由として `disconnected()` に渡す。
- origin は指定しない。`QWebSocket` は origin を指定しなければ Origin ヘッダを送らないので、`server::ws_allowed_origins` の影響を受けない。この挙動はテスト項目 10 で確かめる。

### 4.4 MonitorClient

コマンドを組み立てる関数 (`sendKickOff()` など) はそのまま残し、`sendCommand()` の送信先だけをトランスポートに替える。

```cpp
MonitorClient( QObject * parent,
               DispHolder & disp_holder,
               MonitorTransport * transport, // 所有権は MonitorClient に移る
               const int version );

//! 接続を始める。その場で分かる失敗のときだけ false を返す
bool start( const QString & host, quint16 port, QString * error );

signals:
    void received();
    void disconnectRequested();
    void reconnectRequested();
    void errorMessage( const QString & msg ); // 新規。ステータスバー表示用
```

- **開始:** `start()` は、プロトコルのバージョンとトランスポートの組み合わせを検査してから `open()` を呼ぶ。TCP / WebSocket で v1 / v2 が指定されていたらエラーにする。サーバのテキスト応答をバイナリ構造体として読んでしまうのを防ぐため。
- **生存確認:** タイマは `open()` の直後に始める。TCP の接続待ちが長引いた場合も、このタイマで打ち切れる。タイムアウト時の動作は現状と同じ。
- **dispinit:** `connected()` を受けてから送る。`MainWindow` からの直接呼び出しはやめる。
- **受信:** `messagesAvailable()` を受けたら `popMessage()` で全件を取り出し、1 件ずつ振り分け関数に渡す。1 件以上あればタイマをリセットし、`received()` を 1 回出す。
- **振り分け:**
  - 接続後の最初のメッセージが `(error` で始まるときは接続失敗とし、`errorMessage()` を出して切断を要求する。2 件目以降の `(error ...)` は、従来どおりログに出すだけにする。
  - JSON (version -1) は `addJSON()` に渡す。
  - v3 以上は `addDispInfoV3()` に渡す。
  - v2 / v1 は、ゼロ初期化した構造体に `min( size, sizeof )` バイトをコピーし、`addDispInfoV2()` / `addDispInfoV1()` に渡す。
- **切断:** トランスポートの `disconnected()` を受けたら、`errorMessage()` で理由を出す。そのうえで、自動再接続モードなら `reconnectRequested()`、そうでなければ `disconnectRequested()` を出す。
- **終了:** `disconnect()` はタイマを止め、接続中なら `(dispbye)` を送ってから `close()` する。dispbye が届かなくてもサーバは切断を検知するので、送れなくても問題はない。
- **isConnected():** `transport->isOpen()` を返す。

### 4.5 MainWindow

`connectMonitorTo()`:

1. `Options` から接続方式を取得し、`create_monitor_transport()` でトランスポートを作る。WebSocket 非対応ビルドなどで失敗したら、ステータスバーに理由を出して終える。
2. `MonitorClient` を作り、シグナルを接続してから `start()` を呼ぶ。false が返ったら理由を表示し、クライアントを削除する。
3. データの消去、アクションの有効化、ライブモードへの切り替えは現状どおり行う。`sendDispInit()` の直接呼び出しは削除する。
4. 「Connect to ...」の表示には `peerName()` を使い、接続方式が分かるようにする。

`disconnectMonitor()`:

- `delete` を `deleteLater()` に替える。
- シグナルは直接接続のままにする。キュー接続にすると、切断後に古いクライアントの通知が届き、新しい接続を切ってしまうおそれがあるため。

新しいスロットを 1 つ追加し、`errorMessage()` をステータスバーへの表示につなぐ。

### 4.6 オプション

| 設定 | コマンドライン | QSettings のキー | 既定値 |
|---|---|---|---|
| 接続方式 | `--server-transport udp\|tcp\|ws` | `server_transport` | `udp` |
| WebSocket のパス | `--server-ws-path PATH` | `server_ws_path` | `/monitor` |
| ポート番号 | `--server-port PORT` (既存) | `server_port` (既存) | 6000 |

- `options.cpp` の 3 か所すべてに追加する。`USE_Q_COMMAND_LINE_PARSER` 側、`HAVE_BOOST_PROGRAM_OPTIONS` 側、QSettings の読み書きの 3 か所。
- 方式名は大文字小文字を区別しない。`websocket` も `ws` の別名として受け付ける。不正な値はコマンドラインの解析時にエラーにする。
- ポート番号は全方式で共通とする。新サーバの TCP / WebSocket ポートには既定値がないため、方式ごとの既定値は設けない。

使い方の例:

```sh
rcssserver server::tcp_port=6000 server::ws_port=6080

rcssmonitor --connect                                              # UDP (従来どおり)
rcssmonitor --connect --server-transport tcp --server-port 6000   # TCP
rcssmonitor --connect --server-transport ws  --server-port 6080   # WebSocket
```

### 4.7 ビルドシステム

**CMake**

ルートの `CMakeLists.txt`:

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

`src/CMakeLists.txt`: 新しいソースを `add_executable` に追加し、WebSocket 実装は条件付きで加える。moc は `CMAKE_AUTOMOC` が処理する。

→ 2026-11-18: develop の CMake に合わせて、`Qt${QT_VERSION_MAJOR}::WebSockets` (Qt5 か Qt6) を探し、新しいソースはライブラリ `rcssmonitor_core` に足す (§10)。

```cmake
if(HAVE_QT_WEBSOCKETS)
  target_sources(rcssmonitor PRIVATE ws_monitor_transport.cpp)
  target_link_libraries(rcssmonitor PRIVATE Qt5::WebSockets)
endif()
```

**autotools**

- `m4/ax_have_qt.m4`: このマクロは qmake で利用可能なモジュールを全部リンクするが、一覧に websockets がない。`qtHaveModule(websockets): QT += websockets` を追加する。
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

- `src/Makefile.am`: 新しいソース、ヘッダ、moc ファイルを一覧に追加する。WebSocket 実装は条件付きで加える。

```make
if HAVE_QT_WEBSOCKETS
rcssmonitor_SOURCES += ws_monitor_transport.cpp
nodist_rcssmonitor_SOURCES += moc_ws_monitor_transport.cpp
endif
```

**qmake**

→ 2026-11-18: qmake のビルドは develop で廃止した (3D 表示の計画の D11)。`src/src.pro` は消す (§10)。

`src/src.pro`:

```qmake
qtHaveModule(websockets) {
  QT += websockets
  DEFINES += HAVE_QT_WEBSOCKETS
  HEADERS += ws_monitor_transport.h
  SOURCES += ws_monitor_transport.cpp
}
```

**開発環境についての注意**

- 現在の開発マシンには Qt WebSockets が入っていない。`sudo apt install libqt5websockets5-dev` が必要。
- PATH の先頭に anaconda の qmake (Qt 5.15.2) があり、autotools はこれを使う。システムの qmake が先に見つかるようにする。
- CMake でも anaconda の Qt が見つかる可能性がある。必要なら `-DQt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5` で指定する。

## 5. 変更するファイル

新規:

| ファイル | 内容 |
|---|---|
| `src/monitor_transport.h`, `.cpp` | 抽象クラスと生成関数 |
| `src/udp_monitor_transport.h`, `.cpp` | UDP 実装。`MonitorClient` から移す |
| `src/tcp_monitor_transport.h`, `.cpp` | TCP 実装 |
| `src/message_splitter.h`, `.cpp` | `\0` 区切りの分割処理 |
| `src/ws_monitor_transport.h`, `.cpp` | WebSocket 実装。条件付きでビルドする |

変更:

| ファイル | 内容 |
|---|---|
| `src/monitor_client.h`, `.cpp` | トランスポートの利用、受信処理の統合、接続の流れ |
| `src/main_window.h`, `.cpp` | 接続処理の非同期化、`deleteLater()`、エラー表示 |
| `src/options.h`, `.cpp` | 新しいオプション |
| `CMakeLists.txt`, `src/CMakeLists.txt`, `cmake-config.h.in` | CMake |
| `configure.ac`, `m4/ax_have_qt.m4`, `src/Makefile.am` | autotools |
| `src/src.pro` | qmake |
| `README.md`, `NEWS` | 使い方と変更履歴 |

## 6. 実装手順

各段階で動く状態を保ち、段階ごとにコミットできる順にする。

### 段階 1: UDP のまま抽象化する

- `monitor_transport` と `udp_monitor_transport` を追加する。
- `MonitorClient` をトランスポート経由に書き換え、受信ループを 1 つにまとめる。配列外書き込みもここで直す。
- `start()`、`connected()` を受けての dispinit、`deleteLater()`、`errorMessage()` の表示を入れる。
- 3 つのビルドシステムに新しいファイルを追加する。

完了条件: 既存のサーバに UDP で接続し、JSON・v3・v4・v5 の表示とコマンド送信、自動再接続、自動終了が従来どおり動く。

### 段階 2: TCP と接続方式のオプションを追加する

- `message_splitter` と `tcp_monitor_transport` を追加する。
- `--server-transport` と `--server-ws-path` を追加する。この段階では `ws` を指定するとエラーにする。
- TCP / WebSocket での v1 / v2 の検査を入れる。

完了条件: 新サーバを `server::tcp_port=6000` で起動し、7 章の確認項目が TCP で通る。`MessageSplitter` の単体テストが通る。

### 段階 3: WebSocket を追加する

- `ws_monitor_transport` を追加する。
- 3 つのビルドシステムに Qt WebSockets の検出を入れる。

完了条件: Qt WebSockets がある環境とない環境の両方でビルドが通る。ある環境のビルドで、7 章の確認項目が WebSocket で通る。ない環境のビルドでは、`--server-transport ws` を指定するとエラーが表示される。

### 段階 4: 文書を更新する

- README に使い方とサーバ側の設定例を書く。NEWS と `--help` の説明を更新する。

完了条件: README の手順どおりに、新サーバへ TCP と WebSocket で接続できる。

## 7. テスト計画

### 7.1 確認項目

| # | 項目 | UDP | TCP | WebSocket |
|---|---|---|---|---|
| 1 | JSON・v3・v4・v5 で試合が表示される | ○ | ○ | ○ |
| 2 | v1 / v2 を指定したときの動作 | 従来どおり | 接続前にエラー | 接続前にエラー |
| 3 | キックオフ、ドロップボール、フリーキック、カード、トレーナーの移動、プレイモード変更がサーバに届く | ○ | ○ | ○ |
| 4 | サーバ未起動のとき、通常はエラー表示、自動再接続モードでは再試行が続く | ○ | ○ | ○ |
| 5 | 試合中にサーバを止めると切断として扱われる | 5 秒後 | すぐに | すぐに |
| 6 | 切断操作の後、サーバのログでモニタの離脱が確認できる | ○ | ○ | ○ |
| 7 | `server::max_monitors` を超えるとエラーが表示される | ○ | ○ | ○ |
| 8 | 不正なトレーナーコマンドへの `(error illegal_command_form)` では切断されない | ○ | ○ | ○ |
| 9 | `--server-ws-path /player` でも接続でき、存在しないパスではエラーになる | - | - | ○ |
| 10 | `server::ws_allowed_origins` を設定したサーバにも接続できる | - | - | ○ |
| 11 | 自動終了モードが従来どおり動く | ○ | ○ | ○ |
| 12 | ログファイルの再生に影響がない | ○ | - | - |

### 7.2 単体テスト

`MessageSplitter` について、次の場合を確かめる。

- 1 つのメッセージが複数回の読み込みに分かれて届く。
- 1 回の読み込みに複数のメッセージが含まれる。
- `\0` が連続する場合、空のメッセージを捨てる。
- 未完了のメッセージが上限を超えるとエラーになる。

プロジェクトには既存のテスト基盤がないので、Qt Test を使う小さなテスト実行ファイルを CMake にだけ追加する (`enable_testing()` と `add_test()`)。 → 2026-11-18: develop にテストの仕組みができたので、`rcssmonitor_add_test` で足す。TCP と WebSocket の通信路のテスト (手元のサーバとつなぐ) も足した (§10)。

### 7.3 送信内容の確認

モニタが TCP で送るバイト列は、サーバの代わりに nc で受けて確かめられる。`(dispinit version 5)` の後に `00` が続いていればよい。

```sh
nc -l 6000 | xxd
rcssmonitor --connect --server-transport tcp --server-port 6000
```

## 8. 未決事項

| # | 項目 | 選択肢 | 推奨 |
|---|---|---|---|
| 1 | オプションの形式 | A: `--server-transport` を追加する (本計画)。B: `--server tcp://host:6000` のような URL 形式にする | A。既存の `--server-host` / `--server-port` と設定ファイルの互換性を保てる。URL 形式は、後から「Connect to...」ダイアログの入力欄で受け付けられる |
| 2 | TCP / WebSocket での無通信タイムアウト | 残す / 外す | 残す。切断はソケットで分かるが、サーバが固まった場合はタイムアウトでしか気づけない |
| 3 | ポート番号の既定値 | 全方式で共通の 6000 / 方式ごとに別の値 | 共通。新サーバの TCP / WebSocket ポートには既定値がない |

## 9. 対象外

- **wss:// (TLS):** 新サーバ自体が TLS に対応していない。リバースプロキシ経由での利用は将来の課題とする。
- **Qt 6 への移行** → develop で済んだ。取り込みで、このブランチで足したファイルも Qt6 に対応させた (§10)。
- **圧縮:** TCP / WebSocket では、サーバ側でも使えない。
- **「Connect to...」ダイアログでの方式選択:** 未決事項 1 の結論しだいで追加する。 → 2026-11-06 に作った (45bca98)。オプションは A のまま。
- **CI (AppImage の自動作成とリリース):** master にだけある `.github/workflows/main.yml` と `utils/appimage/` は、master からも削除する予定なので扱わない。このブランチには取り込まない。

## 10. 記録

区切りごとに、日付、決めたこと、作ったもの、見つけた不具合とその原因、試験を書き足す。

### 2026-11-06: 段階 1〜4 (`feature/support-tcp-ws`)

develop (01f880a、19.0.1) から作った `feature/support-tcp-ws` に、5 つのコミットで作った: MonitorTransport と UDP の通信路 (af2e1af)、TCP の通信路と方式のオプション (a41d8d9)、WebSocket の通信路 (6759e11)、文書 (78f1c15)、「Connect to ...」ダイアログでの方式の選択 (45bca98。§9 では対象外としていたが作った)。単体テストは `MessageSplitter` (`test/test_message_splitter.cpp`)。

### 2026-11-18: develop への取り込み (`feature/support-tcp-ws`)

**決めたこと (ユーザ)**
- TCP / WebSocket 対応を公式に採用する見込みになったので、3D 表示の計画の Phase 10 の前に develop に取り込む。rcssserver 側の TCP / WebSocket 対応も develop に入った (8f1097c)。
- 作業は `feature/support-tcp-ws` の上で直接行う。develop をこのブランチにマージし (プッシュ済みなので rebase はしない)、ユーザが develop への PR を作る (3D 表示の計画の §3.5)。
- この計画書をリポジトリに入れる。リポジトリの外に 2 つの写しがあった (直下の `TRANSPORT_PLAN.md`、2026-11-06 22:30 と、`docs/TRANSPORT_PLAN.md`、12:08)。直下のほうが新しく、AppImage の CI の項目を除き、扱わない理由を §9 に書き足したもの。ブランチのコミットにも CI の変更はないので、実装と合っている。これを入れ、英語版を作った。
- 計画書の置き場所を `docs/plan/` にし、ファイル名を `<計画書の作成日>_<計画を表す単語>.md` にそろえる。英語版は `.en.md`。日付は `YYYY-MM-DD` とした (Claude)。この計画書は `2026-11-06_transport.md`。3D 表示の計画書にも後で同じ形を当てる。 → 2026-11-21: CLAUDE.md の形 (ユーザ) に合わせ、英語版は `_en.md` にした (この計画書の英語版は `2026-11-06_transport_en.md`)。3D 表示の計画書も `docs/plan/2026-11-11_3d-view.md` に移した。

**develop をマージした**
- develop (2a88791) は、Phase 0〜3 (CI とテストの仕組み、不具合の修正、JSON パーサの改善、Qt6 対応、qmake の廃止) を含む。衝突は 7 つで、次のように解いた。
  - `CMakeLists.txt`: develop の形 (デスクトップのファイル、Qt5 か Qt6 の Test)。WebSockets を `Qt${QT_VERSION_MAJOR}` で探す。
  - `src/CMakeLists.txt`: 新しいソースを `rcssmonitor_core` に足し、`ws_monitor_transport.cpp` と `Qt${QT_VERSION_MAJOR}::WebSockets` も `rcssmonitor_core` に (WebSockets があるとき)。
  - `test/CMakeLists.txt`: develop の形に、`rcssmonitor_add_test(message_splitter rcssmonitor_core)`。
  - `src/Makefile.am`: 両方のソースとヘッダ。`src/src.pro`: 消した (qmake の廃止)。`src/main_window.cpp`: 両方の include。
  - `README.md`: develop の手順 (CMake を主に、Ubuntu 22.04 / 24.04 / 26.04 の Qt6 と Qt5) に、Qt WebSockets のパッケージ (`qt6-websockets-dev`、`libqt5websockets5-dev`) と、CMake のオプションの一覧に `ENABLE_WEBSOCKET`。
- Qt6 と Qt5 (`-Werror=deprecated-declarations`) でビルドでき、テスト 14 個が通った。

**見つけた不具合と直したこと**
- Qt6 では、TCP の接続の失敗 (サーバがない、など) に気づかなかった。TCP の通信路は、エラーのシグナルを名前でつないでいた (`SIGNAL( error( QAbstractSocket::SocketError ) )`)。Qt6 にはこのシグナルがない (Qt 5.15 で `errorOccurred()` に改名)。名前でつなぐとビルドは通り、実行時に `QObject::connect: No such signal QTcpSocket::error( QAbstractSocket::SocketError )` と出るだけで、エラーが届かない (生存確認のタイマの 5 秒で切れる)。`errorOccurred()` を関数でつないだ (シグナルがなければビルドが止まる)。WebSocket の通信路も、QWebSocket のエラーのシグナルを関数でつないだ (Qt 6.5 から `errorOccurred()`、それより前は `error()`)。
- テスト `monitor_transport` (新): 手元の `QTcpServer` と `QWebSocketServer` につなぐ。TCP: 送るメッセージの末尾に NUL、受けたものを NUL で分ける (空のものは捨てる、分かれて届いたもの)、サーバが閉じると 1 回だけ知らせる、こちらから閉じると知らせずにサーバへ最後のメッセージが届く、つながらない (サーバがない) と理由を知らせる。WebSocket: 要求のパス `/monitor`、Origin のヘッダなし、NUL なしのテキストのメッセージ、受けたもの、サーバが閉じると 1 回、つながらないと理由。WebSocket なしのビルドでは、WebSocket の通信路を作らず理由を返す。名前でつないだシグナルやスロットがないときの Qt の警告も、テストの失敗にする。TCP のエラーを名前でつなぐ形に戻すと、Qt6 で失敗することを確かめた (Qt5 では元から動くので通る)。

**CI**: CMake の 3 つのジョブ (Qt5、Qt6、Ubuntu 22.04 の Qt 6.2) に Qt WebSockets のパッケージを入れ、`config.h` に `HAVE_QT_WEBSOCKETS` があることを確かめる。autotools のジョブは、libdeflate と同じく WebSockets なしでビルドし、WebSocket のない形もコンパイルする。ビルドの並列の数を `$(nproc)` にした (3D 表示のブランチと同じ。数を指定しないと、Makefile のビルドは数の制限なく並列にし、ランナーのメモリが足りなくなりうる)。

**試験**
- Qt 6.4.2 と Qt 5.15.13 (`-Werror=deprecated-declarations`) で、ビルドとテスト 15 個が通る。Qt6 で WebSockets なし (`-DENABLE_WEBSOCKET=OFF`) も通る。autotools (Qt5) は、WebSockets ありと `--disable-websocket` の両方でビルドできる。
- rcssserver の develop (8f1097c) を scratchpad でビルドし、一時的な `RCSS_CONF_DIR` とふだんと違うポート (UDP 16000、TCP 16100、WebSocket 16180) で動かした。`MonitorClient` を使う確認用のプログラムで、UDP、TCP、WebSocket のそれぞれ、版 -1 (JSON) と 5 で、Qt6 と Qt5 の両方で、つながり、フレームが届き (1.5 秒で 15)、キックオフのコマンドでプレイモードが変わり、切断のコマンドを送れた。TCP で版 2 を指定すると、つなぐ前に理由を出して断る。画面での確認 (2D と 3D) はユーザが行う。

**CI の不具合**: 5 コミット (4c0b384〜3bafa02) をプッシュした。CI は、Ubuntu 22.04 (Qt 6.2) のジョブだけが、パッケージを入れる段階で `Unable to locate package qt6-websockets-dev` で落ちた。22.04 では、Qt6 の WebSockets の開発のパッケージは `libqt6websockets6-dev` (6.2.4) で、`qt6-websockets-dev` は 24.04 からの名前 (packages.ubuntu.com で確かめた)。CI のそのジョブと README の手順 (22.04 の名前を添えた) を直した。

### 2026-11-18〜21: 取り込みの後

- ユーザが、PR の前にこのブランチの 2D の画面で、UDP、TCP、WebSocket のライブ接続を確かめた。PR #68 で develop に取り込んだ (Merge Commit、a6df084。2026-11-18)。
- develop を `feature/3d-view` に取り込んだ (c6dc968。2026-11-21)。3D の表示との組み合わせ (ツールバーの接続のボタンを、web 版と同じく接続のダイアログにしたこと、3D のライブの描き方が TCP と WebSocket でも UDP と同じになめらかなこと) は、3D 表示の計画書 (`docs/plan/2026-11-11_3d-view.md`) の §14 に記録した。
- 英語版の名前を CLAUDE.md の形に合わせて `2026-11-06_transport_en.md` にした (2026-11-21)。
- ユーザが `feature/3d-view` で、UDP、TCP、WebSocket のライブ接続を 2D と 3D で確かめた (2026-11-22)。取り込みは済。

### 2026-11-24: 冒頭の表と進み具合の表 (書式の統一)

- 計画書の書式の統一 (CLAUDE.md の「計画書の書式」。URL とログの一覧の計画書の 2026-11-24 の記録) に合わせて、冒頭の表に最終更新日とブランチを足し、状態を短くし、項目の順をそろえて、進み具合の表を置いた。章の構成は変えていない。
