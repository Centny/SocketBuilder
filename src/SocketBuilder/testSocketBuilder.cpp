/*
 * testSocketBuilder.cpp
 *
 *  Created on: May 8, 2013
 *      Author: Scorpion
 */
#include "SocketBuilder.h"
using namespace SocketBuilder;
class UDPProcessImpl: public UDPProcess {
public:
	string name;
	int idx;
	char buf[1024];
	size_t blen;
public:
	UDPProcessImpl(string n) {
		name = n;
		idx = 0;
		blen = 0;
	}
	UDPProcess::UDPProcessEnd execute(char* buf, size_t len,
			ip::udp::endpoint& ep, boost::system::error_code ec) {
		buf[len] = 0;
		printf("%s:%s\n", name.c_str(), buf);
		blen = sprintf(buf, "%s%d", name.c_str(), idx);
		if ("Client" == name) {
			this->builder->send(boost::asio::buffer(buf, blen));
		} else {
			this->builder->send(boost::asio::buffer(buf, blen), ep);
		}
		idx++;
		sleep(1);
		return UDPProcess::UDPProcessReceive;
	}
	void timeout(boost::system::error_code ec) {
		printf("%s:timeout\n", name.c_str());
		this->builder->shutdonw();
	}
};
void testUDPClient() {
	io_service ios;
	UDPProcessImpl upi("Client");
	UDPBuilder b(ios, "127.0.0.1", 50000);
	b.setProcess(&upi);
	b.setSocketTimeout(10000);
	b.startReceive();
	b.send(boost::asio::buffer("start"));
	ios.run();
}
void testUDPServer() {
	io_service ios;
	UDPProcessImpl upi("Server");
	UDPBuilder b(ios, 50000);
//	b.setProcess(&upi);
	b.startReceive();
	ios.run();
}
void testSocketBuilder(int argc, char** argv) {
	if (argc < 2) {
		return;
	}
	if ('c' == argv[1][0]) {
		testUDPClient();
	} else if ('s' == argv[1][0]) {
		testUDPServer();
	}
}
