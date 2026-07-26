# BitTorrent Client

A simple, multi-threaded BitTorrent client written in C++. It parses `.torrent` files, contacts a tracker, connects to peers over TCP, and downloads a single-file torrent using a piece-based strategy.

## Requirements

- CMake 3.16+
- A C++17 compiler (GCC, Clang, or MSVC with WSL)
- [libcurl](https://curl.se/) development headers
- Git (used by CMake to fetch `cxxopts` and `cpr`)

### Linux (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake git libcurl4-openssl-dev
```

### macOS

```bash
brew install cmake curl
```

### Windows

This project uses POSIX networking (`unistd.h`, `arpa/inet.h`, etc.). Build and run it in **WSL** or another Linux environment.

## Build

```bash
git clone <repository-url>
cd BitTorrent
cmake -S . -B build
cmake --build build
```

The executable is created at `build/BitTorrentClient`.

## Run

```bash
./build/BitTorrentClient \
  --torrent-file path/to/file.torrent \
  --output-dir ./downloads \
  --thread-num 5
```

### CLI options

| Option | Short | Description | Default |
|--------|-------|-------------|---------|
| `--torrent-file` | `-t` | Path to the `.torrent` file | required |
| `--output-dir` | `-o` | Directory where the file is saved | required |
| `--thread-num` | `-n` | Number of peer worker threads | `5` |
| `--logging` | `-l` | Enable file logging | `false` |
| `--log-file` | `-f` | Log file path | `../logs/client.log` |
| `--help` | `-h` | Show help | |

Example with logging enabled:

```bash
mkdir -p logs downloads
./build/BitTorrentClient \
  -t sample.torrent \
  -o ./downloads \
  -n 8 \
  -l true \
  -f ./logs/client.log
```

## Generate API docs (Doxygen)

Install Doxygen, then run:

```bash
doxygen Doxyfile
```

Open `docs/html/index.html` in a browser.

## Project layout

```
BitTorrent/
├── CMakeLists.txt
├── README.md
├── src/                 # Client application code
│   ├── main.cpp         # CLI entry point
│   ├── Client.*         # Download orchestration
│   ├── RetrivePeers.*   # Tracker communication
│   ├── ConnectToPeer.*  # Peer protocol handling
│   ├── PieceManager.*   # Piece/block scheduling
│   └── ...
└── lib/
    ├── bencode/         # Bencode parser/encoder
    ├── crypto/          # SHA-1 helper
    └── spdlog/          # Logging (header-only, vendored)
```

## Notes

- Supports single-file torrents only.
- Peer and tracker networking require a POSIX environment.
- First-time CMake configure downloads third-party dependencies (`cxxopts`, `cpr`).
