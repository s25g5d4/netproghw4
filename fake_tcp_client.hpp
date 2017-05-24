#ifndef __FAKE_TCP_HPP__
#define __FAKE_TCP_HPP__

#include <iostream>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "TCPstruct.hpp"

class FakeTCP {
public:

	const static int mss = MSS;
	int cwnd;
	int ssthresh;
	int rtt;
	int dup_ack;
	addrinfo *serverinfo;
	addrinfo *clientinfo;
	addrinfo hints;
	int sockfd;
	uint16_t server_port;
	uint16_t client_port;
	sockaddr_storage clientaddr;
	sockaddr_storage serveraddr;
	socklen_t addr_size;
	uint32_t ack;
	uint32_t seq;
	int rwnd;
	int length;

	FakeTCP(const char * serveraddr, const char * serverport, const char * clientport, int buf_length);

	~FakeTCP();

	bool establish();

	void finish();

	void send(uint8_t * data, int length);

private:

	int _send(const TCPstruct::TCPpacket & data);

	int _recieve(TCPstruct::TCPpacket & data);
};

#endif
