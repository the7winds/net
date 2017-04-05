g++ \
	-m64 \
	-std=c++11 \
	-I./src \
	-I./src/client/ \
	src/*.cpp src/client/*.cpp \
	-lpthread \
	-o bin/client

g++ \
	-m32 \
	-std=c++11 \
	-I./src \
	-I./src/client/ \
	src/*.cpp src/client/*.cpp \
	-lpthread \
	-o bin/client32

g++ \
	-m64 \
	-std=c++11 \
	-I./src \
	-I./src/server/ \
	src/*.cpp src/server/*.cpp \
	-lpthread \
	-o bin/server
