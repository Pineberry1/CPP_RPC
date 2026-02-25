#!/bin/bash
# Simple build script for RPC Server

echo "=== Building RPC Server ==="

g++ -std=c++17 -Wall -O3 \
    -I./include -I./include/rpc \
    src/main.cpp \
    src/rpc_server.cpp \
    src/rpc_client.cpp \
    src/acceptor.cpp \
    src/connection.cpp \
    src/reactor.cpp \
    src/memory_pool.cpp \
    src/tlv_protocol.cpp \
    src/hash_ring.cpp \
    src/service_discovery.cpp \
    src/channel.cpp \
    -o rpc_server -lpthread

if [ $? -eq 0 ]; then
    echo "Build successful!"
    ls -lh rpc_server
else
    echo "Build failed!"
fi
