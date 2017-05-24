#include "fake_tcp_client.hpp"
#include "defines.h"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>

char printable[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int main(int argc, char *argv[])
{
	if (argc < 4) {
		std::cout << "Usage:" << std::endl;
		std::cout << "\t" << argv[0] << " <ip> <port> <filename>" << std::endl;
		return 0;
	}

	char *filepath = strdup(argv[3]);
	char *filename = basename(filepath);
	size_t namelen = strlen(filename);

	if (namelen >= 256) {
		std::cout << "File name too long (>= 256)." << std::endl;
		return 0;
	}

	std::fstream file(argv[3], std::fstream::in | std::fstream::binary);
	if (!file.is_open()) {
		std::cout << "Fail to open file " << filename << std::endl;
		return 0;
	}

	size_t filesize = static_cast<size_t>(file.seekg(0, file.end).tellg());
	if (filesize > FILELEN_MAX) {
		std::cout << "File too large (>= 100MB)." << std::endl;
		return 0;
	}
	file.seekg(0, file.beg);

	uint8_t *data = new uint8_t[filesize + namelen + 1];
	*data = namelen;
	memcpy(data + 1, filename, namelen);
	file.read(reinterpret_cast<char *>(data + namelen + 1), filesize);

	srand(time(NULL));

	FakeTCP client(argv[1], argv[2], STR(CLIENTPORT), filesize + namelen + 1);
	client.establish();
	client.send(data, filesize + namelen + 1);
	client.finish();

	delete [] data;
	free(filepath);

	return 0;
}
