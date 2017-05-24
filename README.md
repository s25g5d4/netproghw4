netproghw4
========================================================

## Build

`make`

## Usage

For server, please run `./server`. By default, server listens at port 10260.

For client, please run `./client`. You will see a help message:

```
$ ./client
Usage:
        ./client <ip> <port> <filename>
```

Please run command `./client 127.0.0.1 10260 file-to-send`.

## Work

* ✔ Socket connection (i.e., connect and bye commands)
* ✔ File correctness
* ✔ Help message
* ✔ Code comments & README file
* ✔ Bonus: Out‐of‐sequence scenario (both server and client drops packet)

## How It Works

For every packet, a TCP-like packet structure is constructed to provide 3-way
handshaking, sequence/acknowledge number, flow control, congestion control. The
server starts listening at UDP port 10260 and waits for only 1 client at a time
to complete the 3-way handshake, then starts to receive file. After the file
transmission is done, client sends FIN packet to inform server to shutdown the
connection. Once server received the file and shut down the connection, it
starts to wait another client to send file.
