#include "fake_tcp_server.hpp"

#include <iostream>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include "TCPstruct.hpp"

static inline bool test_recv(int sockfd)
{
	fd_set readfds;
	timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 100;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	select(sockfd + 1, &readfds, NULL, NULL, &tv);

	return FD_ISSET(sockfd, &readfds);
}

FakeTCP::FakeTCP(const char * serveraddr, const char * serverport, int buf_length)
{
	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(serveraddr, serverport, &hints, &serverinfo);
	if (status != 0) {
		std::cerr << "getaddrinfo error: %s\n" << gai_strerror(status) << std::endl;
		exit(1);
	}

	sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
	if (sockfd == -1) {
		perror("FakeTCP:socket");
		exit(1);
	}

	server_port = ntohs(reinterpret_cast<sockaddr_in*>(serverinfo->ai_addr)->sin_port);

	status = bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen);
	if (status == -1) {
		perror("FakeTCP:bind");
		exit(1);
	}

	clientinfo = nullptr;
	cwnd = mss;
	ssthresh = 4096;
	rtt = 200;
	dup_ack = 0;
	rwnd = buf_length;
	length = buf_length;

	char server_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(serverinfo->ai_addr)->sin_addr, server_ip, INET_ADDRSTRLEN);

	std::cout << "============Parameter============" << std::endl;
	std::cout << "The RTT delay = " << rtt << " ms" << std::endl;
	std::cout << "The threshold = " << ssthresh << " bytes" << std::endl;
	std::cout << "The MSS = " << mss << " bytes" << std::endl;
	std::cout << "The buffer size = " << 10240 << " bytes" << std::endl;
	std::cout << "Sever's IP is " << server_ip << std::endl;
	std::cout << "Sever's IP is listening on port " << server_port << std::endl;
	std::cout << "=================================" << std::endl;
}

FakeTCP::~FakeTCP()
{
	close(sockfd);
	freeaddrinfo(serverinfo);
	freeaddrinfo(clientinfo);
}

bool FakeTCP::accept()
{
	std::cout << "Listening for client..." << std::endl;

	TCPstruct::TCPpacket handshake;

	if (_receive(handshake) == -1) {
		perror("FakeTCP:accept");
		exit(1);
	}

	std::cout << "========Start the three-way handshake========" << std::endl;

	ack = handshake.seq_num() + 1;
	client_port = handshake.src_port();

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(&clientaddr)->sin_addr, client_ip, INET_ADDRSTRLEN);

	std::cout << "Receive a packet(SYN) from " << client_ip << " : " << client_port << std::endl;
	std::cout << "\tReceive a packet (seq_num = " << handshake.seq_num() << ", ack_num = " << handshake.ack_num() << ")" << std::endl;

	int status = connect(sockfd, reinterpret_cast<sockaddr *>(&clientaddr), sizeof (sockaddr));
	if (status == -1) {
		perror("FakeTCP:accept");
		exit(1);
	}

	seq = static_cast<uint32_t>(rand());
	seq %= 10000;

	handshake.clear();
	handshake.src_port(server_port);
	handshake.dst_port(client_port);
	handshake.flag(TCPstruct::flag::SYN, true);
	handshake.flag(TCPstruct::flag::ACK, true);
	handshake.seq_num(seq);
	handshake.ack_num(ack);
	handshake.offset(5);

	std::cout << "Send a packet(SYN/ACK) to " << client_ip << " : " << client_port << std::endl;

	if (_send(handshake) == -1) {
		perror("FakeTCP:accept");
		exit(1);
	}
	if (_receive(handshake) == -1) {
		perror("FakeTCP:accept");
		exit(1);
	}

	if (handshake.flag(TCPstruct::flag::ACK)) {
		std::cout << "Receive a packet(ACK) from " << client_ip << std::endl;
		std::cout << "\tReceive a packet (seq_num = " << handshake.seq_num() << ", ack_num = " << handshake.ack_num() << ")" << std::endl;
		std::cout << "=======Complete the three-way handshake=====" << std::endl;
		seq += 1;
		return true;
	}
	else {
		return false;
	}
}

void FakeTCP::finish()
{
	std::cout << "========Start the four-way handshake========" << std::endl;

	TCPstruct::TCPpacket handshake;

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(&clientaddr)->sin_addr, client_ip, INET_ADDRSTRLEN);

	handshake.src_port(client_port);
	handshake.src_port(server_port);
	handshake.flag(TCPstruct::flag::ACK, true);
	handshake.seq_num(seq);
	handshake.ack_num(ack);
	handshake.offset(5);

	std::cout << "Send ACK to " << client_ip << " : " << client_port << std::endl;

	if (_send(handshake) == -1) {
		perror("FakeTCP:finish");
		exit(1);
	}

	handshake.src_port(client_port);
	handshake.src_port(server_port);
	handshake.flag(TCPstruct::flag::FIN, true);
	handshake.seq_num(seq);
	handshake.ack_num(ack);
	handshake.offset(5);

	std::cout << "Send FIN to " << client_ip << " : " << client_port << std::endl;

	if (_send(handshake) == -1) {
		perror("FakeTCP:establish");
		exit(1);
	}
	if (_receive(handshake) == -1) {
		perror("FakeTCP:finish");
		exit(1);
	}


	if (handshake.flag(TCPstruct::flag::ACK)) {
		std::cout << "Receive ACK from " << client_ip << " : " << client_port << std::endl;
		std::cout << "\tReceive a packet (seq_num = " << handshake.seq_num() << ", ack_num = " << handshake.ack_num() << ")" << std::endl;
		std::cout << "========Complete the four-way handshake========" << std::endl;
	}
}

void FakeTCP::receive(uint8_t * data, int *recv_count)
{
	TCPstruct::TCPpacket recv_packet;

	int total = 0;
	int last_received = 0;
	int initial_seq = static_cast<int>(ack);
	std::set< std::pair<int, int> > lost_seq;

	while (total < length || !lost_seq.empty()) {
		_receive(recv_packet);
		// std::cout << "\tReceive a packet (seq_num = " << recv_packet.seq_num() << ", ack_num = " << recv_packet.ack_num() << ")" << std::endl;

		if (recv_packet.flag(TCPstruct::flag::FIN)) {
			char client_ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(&clientaddr)->sin_addr, client_ip, INET_ADDRSTRLEN);
			std::cout << "\tReceive FIN from " << client_ip << " : " << recv_packet.src_port() << std::endl;
			break;
		}

		bool loss = false;
		int segement_size = static_cast<int>(recv_packet.segement_size());
		int seq_num = static_cast<int>(recv_packet.seq_num()) - initial_seq;

		if (rand() % 1000 >= 990) {
			std::cout << "\x1b[1;31mEmulate loss at " << seq_num << ".\x1b[m" << std::endl;
			continue;
		}

		bool lost_found = false;

		for (auto it = lost_seq.begin(); it != lost_seq.end(); ++it) {
			if (seq_num == it->first) {
				if (seq_num + segement_size < it->second) {
					lost_seq.insert(std::make_pair(seq_num + segement_size, it->second));
				}
				lost_seq.erase(it);
				lost_found = true;
				break;
			}
			else if (seq_num > it->first && seq_num < it->second) {
				if (seq_num + segement_size >= it->second) {
					lost_seq.insert(std::make_pair(it->first, seq_num));
				}
				else {
					lost_seq.insert(std::make_pair(it->first, seq_num));
					lost_seq.insert(std::make_pair(seq_num + segement_size, it->second));
				}
				lost_seq.erase(it);
				lost_found = true;
				break;
			}
		}

		if (lost_found) {
			std::cout << "\x1b[1;32mLoss at byte " << seq_num << " filled.\x1b[m" << std::endl;
			std::cout << std::endl;
		}

		if (seq_num > last_received) {
			loss = true;
			lost_seq.insert(std::make_pair(last_received, seq_num));
		}

		if (seq_num >= total) {
			if (seq_num + segement_size <= length) {
				std::memcpy(data + seq_num, recv_packet._tcp.segement, segement_size);
				if (static_cast<int>(ack) < seq_num + segement_size + initial_seq) {
					ack = seq_num + segement_size + initial_seq;
					total =  seq_num + segement_size;
					rwnd -= segement_size;
				}
			}
			else {
				std::memcpy(data + seq_num, recv_packet._tcp.segement, length - seq_num);
				ack = length + initial_seq;
				total = length;
			}
			if (last_received < seq_num + segement_size) {
				last_received = seq_num + segement_size;
			}
		}
		else {
			if (lost_found) {
				if (seq_num + segement_size <= length) {
					std::memcpy(data + seq_num, recv_packet._tcp.segement, segement_size);
				}
				else {
					std::memcpy(data + seq_num, recv_packet._tcp.segement, length - seq_num);
				}
				rwnd -= segement_size;
			}
		}

		{
			TCPstruct::TCPpacket ack_packet;

			ack_packet.src_port(client_port);
			ack_packet.dst_port(server_port);
			ack_packet.seq_num(seq);
			if (lost_seq.size() != 0) {
				ack_packet.ack_num(lost_seq.begin()->first + initial_seq);
			}
			else {
				ack_packet.ack_num(ack);
			}
			ack_packet.offset(5);
			ack_packet.flag(TCPstruct::flag::ACK, true);
			ack_packet.recv_window(rwnd);

			if (loss) {
				std::cout << "\x1b[1;33mFound loss at byte " << seq_num << ".\x1b[m" << std::endl;
			}
			_send(ack_packet);
			// std::cout << "\tSend a packet (seq_num = " << ack_packet.seq_num() << ", ack_num = " << ack_packet.ack_num() << ")" << std::endl;
		}
	}

	*recv_count = total;
}


int FakeTCP::_send(const TCPstruct::TCPpacket & data)
{
	addr_size = sizeof clientaddr;
	int send_len = ::send(sockfd, data._tcp.raw_packet, static_cast<int>(data.size()), 0);
	return send_len;
}

int FakeTCP::_receive(TCPstruct::TCPpacket & data)
{
	data.clear();
	addr_size = sizeof clientaddr;
	int recv_len;
	if (clientinfo != nullptr) {
		recv_len = recv(sockfd, data._tcp.raw_packet, sizeof (data._tcp.raw_packet), 0);
	}
	else {
		recv_len = recvfrom(sockfd, data._tcp.raw_packet, sizeof (data._tcp.raw_packet), 0,
					reinterpret_cast<sockaddr*>(&clientaddr), &addr_size);
	}

	if (recv_len == -1) {
		return -1;
	}

	rwnd = data.recv_window();
	data._segement_size = static_cast<unsigned int>(recv_len) - data.offset() * 4;

	return recv_len;
}
