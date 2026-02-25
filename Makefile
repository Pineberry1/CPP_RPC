# High Performance RPC Framework Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native
INCLUDES = -I./include -I./include/rpc

# 查找所有源文件
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

# 目标可执行文件
SERVER = rpc_server
CLIENT = rpc_client_demo

# 依赖库
LIBS = -lpthread -lwsock32 -lws2_32 -lstdc++fs

# 默认目标
all: $(SERVER) $(CLIENT)

# 服务器
$(SERVER): src/main.o src/rpc_server.o src/rpc_client.o src/acceptor.o src/channel.o src/connection.o src/reactor.o src/memory_pool.o src/tlv_protocol.o src/hash_ring.o src/service_discovery.o
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LIBS)

# 客户端
$(CLIENT): src/client_demo.o src/rpc_server.o src/rpc_client.o src/acceptor.o src/channel.o src/connection.o src/reactor.o src/memory_pool.o src/tlv_protocol.o src/hash_ring.o src/service_discovery.o
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LIBS)

# 编译规则
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(SERVER) $(CLIENT) $(OBJS)

# 安装 nlohmann/json 依赖
install-deps:
	sudo apt-get update
	sudo apt-get install -y nlohmann-json3-dev libboost-all-dev

# 测试
test: all
	./$(SERVER) &
	SERVER_PID=$$!
	sleep 1
	./$(CLIENT)
	kill $$SERVER_PID

# 性能测试
perf-test: all
	./$(SERVER) &
	SERVER_PID=$$!
	sleep 1
	./$(CLIENT)
	ab -n 10000 -c 100 http://localhost:8080/
	kill $$SERVER_PID

# 调试版本
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -DDEBUG
debug: clean all

# 运行服务器
run-server: all
	./$(SERVER)

# 运行客户端
run-client: all
	./$(CLIENT)

.PHONY: all clean test perf-test debug run-server run-client install-deps
