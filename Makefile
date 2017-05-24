CC=g++
CXX=g++
CXXFLAGS= -std=c++11 -Wall -g
LDFLAGS=
TARGET1=server
SOURCE1=server.cpp TCPstruct.cpp fake_tcp_server.cpp
TARGET2=client
SOURCE2=client.cpp TCPstruct.cpp fake_tcp_client.cpp
OBJS1=$(SOURCE1:.cpp=.o)
OBJS2=$(SOURCE2:.cpp=.o)

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)

$(TARGET2): $(OBJS2)

clean:
	rm -f $(TARGET1) $(TARGET2) *.o