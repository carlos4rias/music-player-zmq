CC=g++ -std=c++11
#ZMQ_PATH=/home/utp/zmq
ZMQ_PATH=/usr/local
ZMQ_INC=$(ZMQ_PATH)/include
ZMQ_LIB=$(ZMQ_PATH)/lib
LIBS=-lzmq -lzmqpp

all: client server

client: client.cc
	$(CC) -L$(ZMQ_LIB) -I$(ZMQ_INC) client.cc -o client $(LIBS) -lsfml-audio -Wall

server: server.cc
	$(CC) -L$(ZMQ_LIB) -I$(ZMQ_INC) server.cc -o server $(LIBS) -Wall
