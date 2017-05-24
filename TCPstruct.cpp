#include "TCPstruct.hpp"
#include <cstring>

#include <netinet/in.h>

using namespace TCPstruct;

TCPpacket::TCPpacket()
{
	memset(_tcp.raw_packet, 0, sizeof (_tcp.raw_packet));
	_segement_size = 0;
}

void TCPpacket::clear()
{
	memset(_tcp.raw_packet, 0, sizeof (_tcp.raw_packet));
	_segement_size = 0;
}

size_t TCPpacket::size() const
{
	return _segement_size + 20;
}

size_t TCPpacket::segement_size() const
{
	return _segement_size;
}

uint16_t TCPpacket::src_port() const
{
	return ntohs(_tcp.src_port);
}

void TCPpacket::src_port(uint16_t src_port)
{
	_tcp.src_port = htons(src_port);
}

uint16_t TCPpacket::dst_port() const
{
	return ntohs(_tcp.dst_port);
}

void TCPpacket::dst_port(uint16_t dst_port)
{
	_tcp.dst_port = htons(dst_port);
}

uint32_t TCPpacket::seq_num() const
{
	return ntohl(_tcp.seq_num);
}

void TCPpacket::seq_num(uint32_t seq_num)
{
	_tcp.seq_num = htonl(seq_num);
}

uint32_t TCPpacket::ack_num() const
{
	return ntohl(_tcp.ack_num);
}

void TCPpacket::ack_num(uint32_t ack_num)
{
	_tcp.ack_num = htonl(ack_num);
}

uint8_t TCPpacket::offset() const
{
	return _tcp.__offset >> 4;
}

void TCPpacket::offset(uint8_t offset)
{
	_tcp.__offset = offset << 4;
}

bool TCPpacket::flag(TCPstruct::flag flag) const
{
	return static_cast<bool>(_tcp.__flags & static_cast<uint8_t>(flag));
}
void TCPpacket::flag(TCPstruct::flag flag, bool set)
{
	if (set) {
		_tcp.__flags |= static_cast<uint8_t>(flag);
	}
	else {
		_tcp.__flags &= ~static_cast<uint8_t>(flag);
	}
}

uint16_t TCPpacket::recv_window() const
{
	return ntohs(_tcp.recv_window);
}

void TCPpacket::recv_window(uint16_t recv_window)
{
	_tcp.recv_window = htons(recv_window);
}

uint16_t TCPpacket::checksum() const
{
	return ntohs(_tcp.checksum);
}

void TCPpacket::checksum(uint16_t checksum)
{
	_tcp.checksum = htons(checksum);
}

uint16_t TCPpacket::urg_pointer() const
{
	return ntohs(_tcp.urg_pointer);
}

void TCPpacket::urg_pointer(uint16_t urg_pointer)
{
	_tcp.urg_pointer = htons(urg_pointer);
}

const uint8_t * TCPpacket::segement() const
{
	return _tcp.segement;
}

void TCPpacket::segement(const uint8_t * segement, size_t length)
{
	if (length) {
		memcpy(_tcp.segement, segement, sizeof (_tcp.segement));
	}
	_segement_size = length;
}
