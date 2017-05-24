#include "fake_tcp_server.hpp"
#include "defines.h"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>

char printable[] = "().-,0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

int main(int argc, char *argv[])
{
	uint8_t *data = new uint8_t[BUFLEN];


	FakeTCP server(argc >= 2 ? argv[1] : NULL, STR(SERVERPORT), BUFLEN);

	while (server.accept()) {
		int recv_count;
		server.receive(data, &recv_count);
		server.finish();

		size_t namelen = *data;
		char *filename = new char[namelen + 1];
		memcpy(filename, data + 1, namelen);
		filename[namelen] = '\0';

		bool invalid_found = false;
		for (unsigned int i = 0; i < namelen; ++i) {
			if (strchr(printable, filename[i]) == NULL) {
				invalid_found = true;
			}
		}
		if (invalid_found) {
			std::cout << "Invalid filename received!" << std::endl;
		}
		else {
			std::ofstream output(filename, std::ofstream::trunc | std::ofstream::binary);
			output.write(reinterpret_cast<const char *>(data + namelen + 1), recv_count - namelen - 1);
		}
	}

	return 0;
}
