#ifndef __FAKE_TCP_HPP__
#define __FAKE_TCP_HPP__

#include <cstdint>

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
	uint16_t rwnd;
	int length;

	FakeTCP(const char * serveraddr, const char * serverport, int buf_length);

	~FakeTCP();

	void finish();

	bool accept();

	void receive(uint8_t * data, int *recv_count);

private:

	int _send(const TCPstruct::TCPpacket & data);

	int _receive(TCPstruct::TCPpacket & data);
};

#endif
