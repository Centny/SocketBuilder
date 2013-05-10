/*
 * TSocketBuilder.cpp
 *
 *  Created on: May 7, 2013
 *      Author: Scorpion
 */

#include "SocketBuilder.h"

namespace SocketBuilder {
AsioBuilder::AsioBuilder(io_service& isv) :
		iosev(isv) {
}
AsioBuilder::~AsioBuilder() {

}
boost::shared_ptr<deadline_timer> AsioBuilder::deadlineTimer() {
	boost::mutex::scoped_lock lock(this->smutex);
	return boost::shared_ptr<deadline_timer>(new deadline_timer(this->iosev));
}
boost::thread_group& AsioBuilder::thrGrps() {
	return this->tgrps;
}

///////
SocketBase::SocketBase(io_service& isv) :
		iosev(isv) {
	this->rtimer = boost::shared_ptr<deadline_timer>(
			new deadline_timer(this->iosev));
	this->stimeout = 0;
}
SocketBase::~SocketBase() {

}
void SocketBase::lock() {
	this->lmutex.lock();
}
void SocketBase::unlock() {
	this->lmutex.unlock();
}
void SocketBase::startTime() {
	if (this->stimeout < 100) {
		return;
	}
	this->rtimer->expires_from_now(
			boost::posix_time::milliseconds(this->stimeout));
	this->rtimer->async_wait(
			boost::bind(&SocketBase::timeoutHandler_, this, _1));
}
void SocketBase::cancelTime() {
	boost::system::error_code ec;
	this->rtimer->cancel(ec);
}
void SocketBase::setSocketTimeout(long sto) {
	this->stimeout = sto;
}
long SocketBase::socketTimeout() {
	return this->stimeout;
}
void SocketBase::timeoutHandler_(const boost::system::error_code& ec) {
	this->timeoutHandler(ec);
}
//////
TSocket::TSocket(io_service& isv) :
		SocketBase(isv), iosev(isv) {
	this->psocket = boost::shared_ptr<ip::tcp::socket>(
			new ip::tcp::socket(this->iosev));
	this->builder = 0;
	this->blen = 0;
}
TSocket::~TSocket() {

}
//boost::shared_ptr<TSocket> TSocket::create(io_service& isv) {
//	boost::shared_ptr<TSocket> sp(new TSocket(isv));
//	sp->sp = sp;
//	return sp;
//}
///
bool TSocket::connect(string host, int port) {
	ip::tcp::endpoint ep(ip::address_v4::from_string(host), port);
	boost::system::error_code ec;
	this->psocket->connect(ep, ec);
	if (!ec.value()) {
		this->initAdr();
	}
	return !ec.value();
}
void TSocket::initAdr() {
	this->radr = psocket->remote_endpoint().address();
	this->ladr = psocket->local_endpoint().address();
}
void TSocket::shutdown() {
	//this->psocket->cancel();
	this->cancelTime();
	boost::system::error_code ignored_ec;
	this->psocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both,
			ignored_ec);
	this->sp = boost::shared_ptr<TSocket>();
}
void TSocket::startRead(string eoc) {
	assert(eoc.size());
	this->startTime();
	boost::asio::async_read_until(*this->psocket, sbuf, eoc,
			boost::bind(&TSocket::readHandle_, this, sp, _1, _2));
}
bool TSocket::syncWrite(const char* data, size_t len) {
	boost::system::error_code ec;
	this->psocket->write_some(buffer(data, len), ec);
	return ec.value() != 0;
}
int TSocket::syncWrite(const char* data, size_t len,
		boost::system::error_code ec) {
	return this->psocket->write_some(buffer(data, len), ec);
}
boost::shared_ptr<ip::tcp::socket> TSocket::socket() {
	return this->psocket;
}
void TSocket::readHandle_(boost::shared_ptr<TSocket> sp,
		const boost::system::error_code& ec, std::size_t bytes_transfered) {
	this->cancelTime();
	if (ec.value() == ECANCELED) {
		return;
	}
	this->readHandle(sp, this->sbuf, ec, bytes_transfered);
}
void TSocket::readHandle(boost::shared_ptr<TSocket> sp,
		boost::asio::streambuf& buf, const boost::system::error_code& ec,
		std::size_t bytes_transfered) {
}
long TSocket::socketTimeout() {
	return this->builder->socketTimeout();
}
void TSocket::timeoutHandler(const boost::system::error_code& ec) {
	if (ec) {
		return;
	}
	this->shutdown();
}
///
TSocketBuilder::TSocketBuilder(io_service& isv, int port) :
		AsioBuilder(isv), iosev(isv) {
	this->acceptor = boost::shared_ptr<ip::tcp::acceptor>(
			new ip::tcp::acceptor(isv, ip::tcp::endpoint(ip::tcp::v4(), port)));
	this->stimeout = 10000;
	this->eoc = DEFAULT_EOC;
}

TSocketBuilder::~TSocketBuilder() {
}

void TSocketBuilder::setEoc(string eoc) {
	this->eoc = eoc;
}
void TSocketBuilder::setSocketTimeout(long sto) {
	this->stimeout = sto;
}
long TSocketBuilder::socketTimeout() {
	return this->stimeout;
}
void TSocketBuilder::acceptHandler_(boost::shared_ptr<TSocket> socket,
		const boost::system::error_code& ec) {
	this->acceptHandler(socket, ec);
}
//virtual method.
void TSocketBuilder::accept() {
	boost::shared_ptr<TSocket> t = this->createSocket();
	boost::shared_ptr<ip::tcp::socket> rsoc = t->socket();
	t->setSocketTimeout(this->socketTimeout());
	this->acceptor->async_accept(*rsoc,
			boost::bind(&TSocketBuilder::acceptHandler, this, t, _1));
}

void TSocketBuilder::acceptHandler(boost::shared_ptr<TSocket> socket,
		const boost::system::error_code& ec) {
	if (ec) {
		return;
	}
	socket->sp = socket;
	socket->startRead(this->eoc);
	socket->initAdr();
	this->accept();
}
boost::shared_ptr<TSocket> TSocketBuilder::createSocket() {
	boost::shared_ptr<TSocket> t = boost::shared_ptr<TSocket>(
			new TSocket(this->iosev));
	t->builder = this;
	return t;
}
////////////////////
UDPBuilder::UDPBuilder(io_service& isv, int port) :
		AsioBuilder(isv), SocketBase(isv), iosev(isv) {
	this->_resolver = 0;
	this->_destination = 0;
	this->_query = 0;
	this->_socket = new ip::udp::socket(this->iosev,
			ip::udp::endpoint(ip::udp::v4(), port));
	this->_port = port;
	this->process = 0;
	this->blen = 0;
}
UDPBuilder::UDPBuilder(io_service& isv, const char* dest, int port) :
		AsioBuilder(isv), SocketBase(isv), iosev(isv) {
	this->_port = port;
	this->_socket = new ip::udp::socket(this->iosev,
			ip::udp::endpoint(ip::udp::v4(), 0));
	this->_resolver = new ip::udp::resolver(this->iosev);
	this->_destination = new char[strlen(dest)];
	memccpy(this->_destination, dest, sizeof(char), strlen(dest));
	char tmp[255];
	int ts = std::sprintf(tmp, "%d", port);
	if (ts < 255) {
		tmp[ts] = 0;
	} else {
		tmp[254] = 0;
	}
	this->_query = new ip::udp::resolver::query(ip::udp::v4(),
			this->_destination, tmp);
	this->process = 0;
	this->blen = 0;
}
UDPBuilder::~UDPBuilder() {
	//common.
	if (_socket) {
		delete _socket;
		_socket = 0;
	}
	//client
	if (_resolver) {
		delete _resolver;
		_resolver = 0;
	}
	if (_destination) {
		delete _destination;
		_destination = 0;
	}
	if (_query) {
		delete _destination;
		_destination = 0;
	}
}
ip::udp::socket& UDPBuilder::socket() {
	return *_socket;
}
void UDPBuilder::startReceive() {
	if (this->socketTimeout() > 100) {
		this->startTime();
	}
	this->_socket->async_receive_from(boost::asio::buffer(cbuf, BUF_SIZE), ep,
			boost::bind(&UDPBuilder::readHandle_, this, _1, _2));
}
void UDPBuilder::shutdonw() {
	//this->_socket->cancel();
	this->cancelTime();
	boost::system::error_code ignored_ec;
	this->_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both,
			ignored_ec);
}
void UDPBuilder::setProcess(UDPProcess *pro) {
	this->process = pro;
	this->process->builder = this;
}
void UDPBuilder::readHandle_(const boost::system::error_code& ec,
		std::size_t bytes_transfered) {
	this->cancelTime();
	if (ec.value() == ECANCELED) {
		return;
	}
	this->readHandle(cbuf, ec, bytes_transfered);
}
void UDPBuilder::readHandle(char* buf, const boost::system::error_code& ec,
		std::size_t bytes_transfered) {
	if (this->process) {
		UDPProcess::UDPProcessEnd pe = this->process->execute(buf,
				bytes_transfered, ep, ec);
		switch (pe) {
		case UDPProcess::UDPProcessReceive:
			this->startReceive();
			break;
		case UDPProcess::UDPProcessClose:
			this->shutdonw();
			break;
		default:
			break;
		}
	} else {
		this->startReceive();
	}
}
void UDPBuilder::timeoutHandler(const boost::system::error_code& ec) {
	if (ec) {
		return;
	}
	if (this->process) {
		this->process->timeout(ec);
	}
}
} /* namespace SocketBuilder */
