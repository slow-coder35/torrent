# BitTorrent Client

A BitTorrent client implemented from scratch in modern C++. This project implements the core BitTorrent protocol, including `.torrent` file parsing, tracker communication, peer connections, piece downloading, and file reconstruction without relying on external BitTorrent libraries.

> **Status:** Under Active Development

---

## Features

### Torrent Parsing
- Parse `.torrent` files
- Recursive bencode decoder
- Extract torrent metadata
- Compute SHA-1 info hash
- Support for single-file torrents

### Tracker Communication
- HTTP tracker support
- Tracker announce requests
- Compact peer list parsing
- Peer discovery

### Peer Protocol
- TCP peer connections
- BitTorrent handshake
- Bitfield processing
- HAVE message handling
- Interested / Not Interested
- Choke / Unchoke handling
- Piece requests
- Piece reception

### Download Engine
- Piece management
- Block-based downloading
- Random-access file writing using `pwrite`
- Multi-threaded peer communication
- Placeholder file creation

---

## Project Structure

```
.
├── main.cpp
├── torrent.cpp
├── torrent.h
├── torrent_session.cpp
├── torrent_session.h
├── peerconnection.cpp
├── peerconnection.h
├── tracker_client.cpp
├── tracker_client.h
├── bencode.cpp
├── bencode.h
├── file_io.cpp
├── file_io.h
├── utils.cpp
├── utils.h
└── README.md
```

---

## Download Workflow

```
                +----------------+
                | .torrent file  |
                +--------+-------+
                         |
                         v
                Parse Bencode Data
                         |
                         v
                Extract Metadata
                         |
                         v
               Generate Info Hash
                         |
                         v
                Contact Tracker
                         |
                         v
               Receive Peer List
                         |
                         v
             Connect to Available Peers
                         |
                         v
               BitTorrent Handshake
                         |
                         v
              Receive Peer Bitfield
                         |
                         v
               Send Interested Message
                         |
                         v
               Wait for Unchoke Message
                         |
                         v
                 Request Piece Blocks
                         |
                         v
                 Receive Piece Data
                         |
                         v
                  Write Data to File
                         |
                         v
               Repeat Until Complete
```

---

## Supported Messages

| Message | Status |
|----------|--------|
| Handshake | ✅ |
| Keep Alive | ✅ |
| Choke | ✅ |
| Unchoke | ✅ |
| Interested | ✅ |
| Not Interested | ✅ |
| Have | ✅ |
| Bitfield | ✅ |
| Request | ✅ |
| Piece | ✅ |
| Cancel | ❌ |

---

## Technologies Used

- **Language:** C++20
- **Networking:** POSIX Sockets
- **Hashing:** OpenSSL SHA-1
- **Multithreading:** `std::thread`
- **Build System:** CMake

---

## Building

### Using CMake

```bash
git clone https://github.com/<username>/torrent-client.git

cd torrent-client

mkdir build
cd build

cmake ..
make
```

### Using g++

```bash
g++ *.cpp -pthread -lssl -lcrypto -o torrent
```

---

## Running

```bash
./torrent <path-to-torrent-file>
```

Example:

```bash
./torrent ubuntu.torrent
```

---

## Example Output

```
Connecting to tracker...

Tracker returned 2 peers.

Connecting to peer 192.168.29.49...
Handshake successful.

Received bitfield.
Sending Interested message...
Received Unchoke.

Downloading...

Piece 1 completed
Piece 2 completed
Piece 3 completed
...

Download complete.
```

---

## Current Limitations

- Single-file torrents only
- HTTP trackers only
- Basic sequential piece selection
- No upload/seeding support
- No resume support
- No magnet links
- No DHT or Peer Exchange (PEX)

---

## Future Improvements

- Piece SHA-1 verification
- Multi-peer piece scheduling
- Rarest-first piece selection
- End-game mode
- Upload / Seeding
- Resume support
- UDP tracker support
- Magnet links
- DHT
- Peer Exchange (PEX)
- Bandwidth throttling

---

## Learning Objectives

This project was built to understand and implement the BitTorrent protocol from the ground up rather than using an existing BitTorrent library. It covers:

- Binary protocol implementation
- TCP socket programming
- HTTP networking
- File systems
- Multithreading
- Hashing and integrity checking
- Distributed systems concepts
- Concurrent programming

---

## License

This project is licensed under the MIT License.