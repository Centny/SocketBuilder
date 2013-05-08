/*
 * SocketBuilder.h
 *
 *  Created on: May 7, 2013
 *      Author: Scorpion
 */

#ifndef SOCKETBUILDER_H_
#define SOCKETBUILDER_H_
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>
namespace SocketBuilder {
using namespace std;
using namespace boost;
using namespace boost::asio;
class SocketBuilder;
#define bsleep(x) boost::this_thread::sleep(boost::posix_time::milliseconds(x))
#define BUF_SIZE 102400
//#define R_BUF_SIZE 2048
#define DEFAULT_EOC "\r\n"
/*
 * the ASIO builder base.
 */
class AsioBuilder {
private:
	io_service& iosev;
	boost::thread_group tgrps;
	long stimeout;
	boost::mutex smutex;
public:
	AsioBuilder(io_service& isv);
	virtual ~AsioBuilder();
	boost::shared_ptr<deadline_timer> deadlineTimer();
	boost::thread_group& thrGrps();
	void setSocketTimeout(long sto);
	virtual long socketTimeout();
};
/*
 * the ASIO socket base.
 */
class SocketBase {
public:
	class ScopedLock {
	private:
		SocketBase* c;
	public:
		ScopedLock(SocketBase* c) {
			this->c = c;
			this->c->lock();
		}
		~ScopedLock() {
			this->c->unlock();
			this->c = 0;
		}
	};
private:
	io_service& iosev;
	boost::mutex lmutex;
#define LM_LOCK	boost::mutex::scoped_lock lmlock(this->lmutex)
	//
	boost::shared_ptr<deadline_timer> rtimer;
public:
	SocketBase(io_service& isv);
	virtual ~SocketBase();
	void lock();
	void unlock();
	void startTime();
	void cancelTime();
private:
	void timeoutHandler_(const boost::system::error_code& ec);
protected:
	virtual void timeoutHandler(const boost::system::error_code& ec)=0;
	virtual long socketTimeout()=0;
};
/*
 *
 */
class TSocket: public SocketBase {
private:
	io_service& iosev;
protected:
	//
	boost::asio::ip::address radr;
	boost::asio::ip::address ladr;
	char cbuf[BUF_SIZE];
	boost::asio::streambuf sbuf;
	//
	boost::shared_ptr<ip::tcp::socket> psocket;

public:
	boost::shared_ptr<TSocket> sp;
	SocketBuilder *builder;
public:
	TSocket(io_service& isv);
	virtual ~TSocket();
//	static boost::shared_ptr<TSocket> create(io_service& isv);
	//
	bool connect(string host, int port);
	void initAdr();
	void shutdown();
	void startRead(string eoc = DEFAULT_EOC);
	//
	bool syncWrite(const char* data, size_t len);
	int syncWrite(const char* data, size_t len, boost::system::error_code ec);
	boost::shared_ptr<ip::tcp::socket> socket();
	//
private:
	void readHandle_(boost::shared_ptr<TSocket> sp,
			const boost::system::error_code& ec, std::size_t bytes_transfered);
protected:
	//overide method.
	virtual void readHandle(boost::shared_ptr<TSocket> sp,
			boost::asio::streambuf& buf, const boost::system::error_code& ec,
			std::size_t bytes_transfered);
	virtual long socketTimeout();
	virtual void timeoutHandler(const boost::system::error_code& ec);
};

/*
 *
 */
class SocketBuilder: public AsioBuilder, public SocketBase {
private:
	io_service& iosev;
protected:
	char cbuf[BUF_SIZE];
	boost::asio::streambuf sbuf;
	boost::shared_ptr<ip::tcp::acceptor> acceptor;
	string eoc;
public:
	SocketBuilder(io_service& isv, int port);
	virtual ~SocketBuilder();
	//
	void setEoc(string eoc);
private:
	void acceptHandler_(boost::shared_ptr<TSocket> socket,
			const boost::system::error_code& ec);
public:
	virtual void accept();
protected:
	virtual void acceptHandler(boost::shared_ptr<TSocket> socket,
			const boost::system::error_code& ec);
	virtual boost::shared_ptr<TSocket> createSocket();
};
/*
 *
 */
class UDPBuilder;
class UDPProcess {
public:
	/*
	 * execute extend after process.
	 */
	enum UDPProcessEnd {
		UDPProcessNone = 1,	//do nothing.
		UDPProcessReceive = 2,	//start receive.
		UDPProcessClose = 3 //close
	};
	friend class UDPBuilder;
protected:
	UDPBuilder *builder;
public:
	virtual ~UDPProcess() {
	}
	virtual UDPProcessEnd execute(boost::asio::streambuf buf,
			ip::udp::endpoint& ep, boost::system::error_code ec)=0;
};
class UDPBuilder: public AsioBuilder {
private:
	io_service& iosev;
protected:
	//common
	//
	char cbuf[BUF_SIZE];
	boost::asio::streambuf sbuf;
	ip::udp::socket *_socket;
	short _port;
	//client
	ip::udp::resolver *_resolver;
	ip::udp::resolver::query *_query;
	char *_destination;
private:
	ip::udp::endpoint ep;
public:
	//server
	UDPBuilder(io_service& isv, short port);
	template<typename T>
	void send(const T& buf, ip::udp::endpoint& ep) {
		_socket->send_to(buf, ep);
	}
	//client
	UDPBuilder(io_service& isv, const char* dest, short port);
	template<typename T>
	void send(const T& buf) {
		ip::udp::resolver::iterator iterator = _resolver->resolve(*_query);
		ip::udp::endpoint ep = (ip::udp::endpoint) (*iterator);
		_socket->send_to(buf, ep);
	}
	ip::udp::socket& socket();
	virtual ~UDPBuilder();
	template<typename T>
	size_t receive(const T& buf, ip::udp::endpoint& ep) {
		return _socket->receive_from(buf, ep);
	}
	virtual void startReceive();
	virtual void shutdonw();
private:
	void readHandle_(const boost::system::error_code& ec,
			std::size_t bytes_transfered);
	void timeoutHandler_(const boost::system::error_code& ec);
protected:
	virtual void readHandle(char* buf, const boost::system::error_code& ec,
			std::size_t bytes_transfered);
	virtual void timeoutHandler(const boost::system::error_code& ec);
};
} /* namespace SocketBuilder */
#endif /* SOCKETBUILDER_H_ */
