# yottorrent 
A lightweight BitTorrent client written in modern C++20, using
Boost.Asio coroutines for fully asynchronous networking.  

This is made as a learning project and is not suitable for daily use as it currently
lacks many features. But I aim to add more features in future.

### Current Features
- HTTP tracker communication (compact and normal peer lists)
- Fast and lightweight Bencode parsing and torrent file handling (hand‑rolled parser)
- BitTorrent protocol handshake (v1)
- Sparse-file pre-allocation for fast seek-and-write.
- Automatic cleanup and block re‑queue on peer disconnect
- Block-level (16 KiB) pipelining with a per-peer backlog.
- Asynchronous multi-peer downloading via boost::asio::co_spawn coroutines.

### Dependencies
- CMake (3.20+)
- Compiler supporting C++20 (GCC 10+, Clang 11+, or MSVC 19.28+)
- Boost (specifically `Boost.Asio` and `Boost.Url`)
- LibCPR (Fetched automatically via CMake)

### Building
```bash
git clone https://github.com/MkP/yottorrent.git
cd yottorrent
cmake -B build
cmake --build build
```

### Run
```bash
./build/yottorrent <path_to_torrent_file> <download_path_for_file>
```

### TODO
- Support UDP trackers (BEP 15)
- Add seeding capability
- Support Multi-file Torrents

