#ifndef __TCPSTRUCT_HPP__
#define __TCPSTRUCT_HPP__

#include "defines.h"

#include <cstdint>
#include <cstdlib>

namespace TCPstruct {
	enum flag {
		URG = 32,
		ACK = 16,
		PSH = 8,
		RST = 4,
		SYN = 2,
		FIN = 1
	};

	class TCPpacket {
		public:
		union {
			struct {
				uint16_t src_port;
				uint16_t dst_port;
				uint32_t seq_num;
				uint32_t ack_num;
				uint8_t  __offset;
				uint8_t  __flags;
				uint16_t recv_window;
				uint16_t checksum;
				uint16_t urg_pointer;
				uint8_t  segement[MSS];
			};
			uint8_t raw_packet[MSS + 20];
		} _tcp;

		size_t _segement_size;

		TCPpacket();

		void clear();
		size_t size() const;
		size_t segement_size() const;

		uint16_t src_port() const;
		void src_port(uint16_t src_port);
		uint16_t dst_port() const;
		void dst_port(uint16_t dst_port);

		uint32_t seq_num() const;
		void seq_num(uint32_t seq_num);

		uint32_t ack_num() const;
		void ack_num(uint32_t ack_num);

		uint8_t offset() const;
		void offset(uint8_t offset);

		bool flag(TCPstruct::flag flag) const;
		void flag(TCPstruct::flag flag, bool set);

		uint16_t recv_window() const;
		void recv_window(uint16_t recv_window);

		uint16_t checksum() const;
		void checksum(uint16_t checksum);

		uint16_t urg_pointer() const;
		void urg_pointer(uint16_t urg_pointer);

		const uint8_t * segement() const;
		void segement(const uint8_t * segement, size_t length);
	};
}

#endif
