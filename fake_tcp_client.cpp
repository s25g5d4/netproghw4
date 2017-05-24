#include "fake_tcp_client.hpp"

#include <algorithm>
#include <utility>
#include <set>

static inline bool test_recv(int sockfd)
{
	fd_set readfds;
	timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	select(sockfd + 1, &readfds, NULL, NULL, &tv);

	return FD_ISSET(sockfd, &readfds);
}

FakeTCP::FakeTCP(const char * serveraddr, const char * serverport, const char * clientport, int buf_length)
{
	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

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

	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, clientport, &hints, &clientinfo);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	status = bind(sockfd, clientinfo->ai_addr, clientinfo->ai_addrlen);
	if (status == -1) {
		perror("FakeTCP:bind");
		exit(1);
	}

	client_port = ntohs(reinterpret_cast<sockaddr_in*>(clientinfo->ai_addr)->sin_port);

	cwnd = mss;
	ssthresh = 65535;
	rtt = 200;
	dup_ack = 0;
	rwnd = buf_length;
	length = buf_length;
}

FakeTCP::~FakeTCP()
{
	close(sockfd);
	freeaddrinfo(serverinfo);
	freeaddrinfo(clientinfo);
}

bool FakeTCP::establish()
{
	int status = connect(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen);
	if (status == -1) {
		perror("FakeTCP:connect");
		return 1;
	}

	std::cout << "========Start the three-way handshake========" << std::endl;

	seq = static_cast<uint32_t>(rand());
	seq %= 10000;

	TCPstruct::TCPpacket handshake;
	handshake.src_port(client_port);
	handshake.dst_port(server_port);
	handshake.flag(TCPstruct::flag::SYN, true);
	handshake.seq_num(seq);
	handshake.offset(5);
	handshake.recv_window(rwnd);

	char server_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(serverinfo->ai_addr)->sin_addr, server_ip, INET_ADDRSTRLEN);
	std::cout << "Send SYN to " << server_ip << " : " << server_port << " seq " << handshake.seq_num() << " ack " << handshake.ack_num() << std::endl;

	if (_send(handshake) == -1) {
		perror("FakeTCP:establish");
		exit(1);
	}
	if (_recieve(handshake) == -1) {
		perror("FakeTCP:establish");
		exit(1);
	}

	if (!handshake.flag(TCPstruct::flag::ACK) || !handshake.flag(TCPstruct::flag::SYN))
		return false;

	std::cout << "Recieve SYN-ACK from " << server_ip << " : " << std::endl;
	std::cout << "\tRecieve a packet (seq_num = " << handshake.seq_num() << ", ack_num = " << handshake.ack_num() << ")" << std::endl;

	seq += 1;
	ack = handshake.seq_num() + 1;

	handshake.clear();
	handshake.src_port(client_port);
	handshake.dst_port(server_port);
	handshake.flag(TCPstruct::flag::ACK, true);
	handshake.seq_num(seq);
	handshake.ack_num(ack);
	handshake.offset(5);
	handshake.recv_window(rwnd);

	std::cout << "Send ACK to " << server_ip << " : " << server_port << std::endl;

	if (_send(handshake) == -1) {
		perror("FakeTCP:establish");
		exit(1);
	}

	std::cout << "=======Complete the three-way handshake=====" << std::endl;

	return true;
}

void FakeTCP::finish()
{
	std::cout << "========Start the four-way handshake========" << std::endl;

	TCPstruct::TCPpacket handshake;
	handshake.src_port(client_port);
	handshake.src_port(server_port);
	handshake.flag(TCPstruct::flag::FIN, true);
	handshake.seq_num(seq);
	handshake.ack_num(ack);
	handshake.offset(5);

	char server_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(&serveraddr)->sin_addr, server_ip, INET_ADDRSTRLEN);

	std::cout << "Send a packet(FIN) to " << server_ip << " : " << server_port << std::endl;

	if (_send(handshake) == -1) {
		perror("FakeTCP:finish");
		exit(1);
	}
	if (_recieve(handshake) == -1) {
		perror("FakeTCP:finsh");
		exit(1);
	}

	if (handshake.flag(TCPstruct::flag::ACK)) {
		std::cout << "Recieve a packet(ACK) from " << server_ip << " : " << server_port << std::endl;
		std::cout << "\tRecieve a packet (seq_num = " << handshake.seq_num() << ", ack_num = " << handshake.ack_num() << ")" << std::endl;
	}

	while (true) {
	if (_recieve(handshake) == -1) {
		perror("FakeTCP:finsh");
		exit(1);
	}

	if (handshake.flag(TCPstruct::flag::FIN)) {
		std::cout << "Recieve a packet(FIN) from " << server_ip << " : " << server_port << std::endl;
		std::cout << "\tRecieve a packet (seq_num = " << handshake.seq_num() << ", ack_num = " << handshake.ack_num() << ")" << std::endl;
		break;
	}
	}

	handshake.src_port(client_port);
	handshake.src_port(server_port);
	handshake.flag(TCPstruct::flag::ACK, true);
	handshake.seq_num(seq);
	handshake.ack_num(ack);
	handshake.offset(5);

	if (_send(handshake) == -1) {
		perror("FakeTCP:finish");
		exit(1);
	}

	std::cout << "Send a packet(ACK) to " << server_ip << " : " << server_port << std::endl;
	std::cout << "========Complete the four-way handshake========" << std::endl;

	clientinfo = nullptr;
	cwnd = mss;
	ssthresh = 4096;
	rtt = 500;
	dup_ack = 0;
}

void FakeTCP::send(uint8_t * data, int length)
{
	TCPstruct::TCPpacket send_packet;

	int initial_ack = seq;
	int last_acked = 0;
	int last_sent = 0;
	int next_sent;
	clock_t timestamp;
	bool had_recieve_ack = true;
	bool congestion_avoidance = true;

	std::cout << std::endl;
	std::cout << "*************Slow start*************" << std::endl;

	while (last_acked < length) {

		if (last_sent - last_acked < cwnd) {
			next_sent = std::min({ length - last_sent, mss, cwnd - (last_sent - last_acked), (rwnd == 0 ? 1 : rwnd) });

			if (next_sent) {
				send_packet.clear();
				send_packet.src_port(client_port);
				send_packet.dst_port(server_port);
				send_packet.seq_num(seq);
				send_packet.ack_num(ack);
				send_packet.offset(5);
				send_packet.segement(data + last_sent, static_cast<size_t>(next_sent));

				if (rand() % 1000 < 995) {
					_send(send_packet);
				}
				else {
					std::cout << std::endl;
					std::cout << "\x1b[1;31mEmulate loss at " << seq << ".\x1b[m" << std::endl;
				}

				if (had_recieve_ack) {
					timestamp = clock();
					had_recieve_ack = false;
				}

				// std::cout << "cwnd = " << cwnd << ", rwnd = " << rwnd << ", threshold = " << ssthresh << std::endl;
				// std::cout << "\tSend a packet at : " << send_packet.segement_size() << " bytes (seq_num = " << send_packet.seq_num() << ", ack_num = " << send_packet.ack_num() << ")" << std::endl;

				last_sent += next_sent;
				seq += next_sent;
			}
		}

		while (test_recv(sockfd)) {
			TCPstruct::TCPpacket ack_packet;
			_recieve(ack_packet);
			// std::cout << "\tRecieve a packet (seq_num = " << ack_packet.seq_num() << ", ack_num = " << ack_packet.ack_num() << ")" << std::endl;

			int ack_num = static_cast<int>(ack_packet.ack_num()) - initial_ack;

			if (ack_num == last_acked) {
				++dup_ack;
				if (dup_ack == 3) {
					std::cout << std::endl;
					std::cout << "*************Fast recovery*************" << std::endl;
					ssthresh = cwnd / 2;
					cwnd = ssthresh + 3 * mss;

					next_sent = std::min({ length - last_acked, mss, static_cast<int>(rwnd) });

					send_packet.clear();
					send_packet.src_port(client_port);
					send_packet.dst_port(server_port);
					send_packet.seq_num(last_acked + initial_ack);
					send_packet.ack_num(ack);
					send_packet.offset(5);
					send_packet.segement(data + last_acked, static_cast<size_t>(next_sent));
					_send(send_packet);

					std::cout << "cwnd = " << cwnd << ", rwnd = " << rwnd << ", threshold = " << ssthresh << std::endl;
					std::cout << "\t\x1b[1;33mResend a packet at byte: " << last_acked << "\x1b[m (seq_num = " << send_packet.seq_num() << ", ack_num = " << send_packet.ack_num() << ")" << std::endl;

					dup_ack = 0;
					congestion_avoidance = true;
				}
			}
			else {
				if (congestion_avoidance && cwnd >= ssthresh) {
					congestion_avoidance = false;
					std::cout << std::endl;
					std::cout << "*************Congestion avoidance*************" << std::endl;
				}
				cwnd += (cwnd >= ssthresh ? mss * mss / cwnd : mss);

				last_acked = ack_num;
				dup_ack = 0;
				had_recieve_ack = true;
			}
		}

		{
			clock_t timeout = clock();

			if ((timeout - timestamp)*1000.0/static_cast<double>(CLOCKS_PER_SEC) > rtt) {
				ssthresh = cwnd / 2;
				cwnd = mss;
				dup_ack = 0;

				next_sent = std::min({ length - last_acked, mss, static_cast<int>(rwnd) });

				send_packet.clear();
				send_packet.src_port(client_port);
				send_packet.dst_port(server_port);
				send_packet.seq_num(last_acked + initial_ack);
				send_packet.ack_num(ack);
				send_packet.offset(5);
				send_packet.segement(data + last_acked, static_cast<size_t>(next_sent));
				_send(send_packet);

				std::cout << "cwnd = " << cwnd << ", rwnd = " << rwnd << ", threshold = " << ssthresh << std::endl;
				std::cout << "\t\x1b[1;33mResend a packet at byte: " << last_acked << "\x1b[m (seq_num = " << send_packet.seq_num() << ", ack_num = " << send_packet.ack_num() << ")" << std::endl;

				timestamp = clock();
			}
		}
	}

	std::cout << std::endl;
	std::cout << "The file transmission is finished." << std::endl;
}

int FakeTCP::_send(const TCPstruct::TCPpacket & data)
{
	addr_size = sizeof clientaddr;
	int send_len = ::send(sockfd, data._tcp.raw_packet, static_cast<int>(data.size()), 0);
	return send_len;
}

int FakeTCP::_recieve(TCPstruct::TCPpacket & data)
{
	data.clear();
	addr_size = sizeof serveraddr;
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
	data._segement_size = static_cast<unsigned int>(recv_len) - data.offset() * 4;

	return recv_len;
}
