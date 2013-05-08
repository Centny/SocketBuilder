/*
 * testSocketBuilder.cpp
 *
 *  Created on: May 8, 2013
 *      Author: Scorpion
 */
#include "SocketBuilder.h"
#include <iostream>
using namespace NetBuilder;
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
class TSocketImpl: public TSocket {
public:
	int idx;
	string name;
public:
	TSocketImpl(io_service& isv) :
			TSocket(isv) {
		idx = 0;
		blen = 0;
	}
	virtual void readHandle(boost::shared_ptr<TSocket> socket,
			boost::asio::streambuf& buf, const boost::system::error_code& ec,
			std::size_t bytes_transfered) {
		if (ec) {
			this->shutdown();
			return;
		}
		//copy new buf to data.
		std::istream ibuf(&buf);
		ibuf.read(cbuf, bytes_transfered);
		cbuf[bytes_transfered] = 0;
		string data(cbuf);
		printf("%s:%s\n", name.c_str(), cbuf);
		if (name == "Client") {
			blen = sprintf(cbuf, "%s%d"DEFAULT_EOC, name.c_str(), idx);
			this->syncWrite(cbuf, blen);
			sleep(1);
		}
		idx++;
		this->startRead();
	}
	virtual void shutdown() {
		printf("shutdown\n");
		TSocket::shutdown();
	}
	virtual ~TSocketImpl() {
		printf("release\n");
	}
	virtual void timeoutHandler(const boost::system::error_code& ec) {
		if (ec) {
			return;
		}
		printf("timeout\n");
		this->shutdown();
	}
};
class SocketBuilderImpl: public SocketBuilder {
public:
	SocketBuilderImpl(io_service& isv, int port) :
			SocketBuilder(isv, port) {
	}
	virtual boost::shared_ptr<TSocket> createSocket() {
		TSocketImpl *tsi = new TSocketImpl(this->iosev);
		tsi->name = "Server";
		return boost::shared_ptr<TSocket>(tsi);
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
	b.setProcess(&upi);
	b.startReceive();
	ios.run();
}
void testTCPClient() {
	io_service ios;
	TSocketImpl tsi(ios);
	tsi.name = "Client";
	assert(tsi.connect("127.0.0.1", 60000));
	tsi.syncWrite("Start"DEFAULT_EOC, 7);
	tsi.startRead();
	ios.run();
}
void testTCPServer() {
	io_service ios;
	SocketBuilderImpl sbi(ios, 60000);
	sbi.accept();
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
	} else if ('C' == argv[1][0]) {
		testTCPClient();
	} else if ('S' == argv[1][0]) {
		testTCPServer();
	}
}
