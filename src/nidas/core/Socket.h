/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_CORE_SOCKET_H
#define NIDAS_CORE_SOCKET_H

#include <nidas/core/IOChannel.h>
#include <nidas/core/DOMable.h>
#include <nidas/util/Socket.h>
#include <nidas/util/Thread.h>

#include <string>
#include <iostream>

namespace nidas { namespace core {

/**
 * Implementation of an IOChannel, over a Socket.
 */
class Socket: public IOChannel {

public:

    /**
     * Constructor.
     */
    Socket();

    /**
     * Copy constructor.
     */
    Socket(const Socket& x);

    /**
     * Constructor from a connected nidas::util::Socket.
     * @param sock Pointer to the connected nidas::util::Socket.
     * Socket will own the pointer and will delete it
     * it its destructor.
     */
    Socket(nidas::util::Socket* sock);

    ~Socket();

    Socket* clone() const;

    void requestConnection(ConnectionRequester* service)
    	throw(nidas::util::IOException);

    IOChannel* connect() throw(nidas::util::IOException);

    virtual bool isNewFile() const { return newFile; }

    /**
     * Do setKeepAliveIdleSecs(int secs) on underlying socket.
     */
    void setKeepAliveIdleSecs(int val) throw (nidas::util::IOException)
    {
	keepAliveIdleSecs = val;
	if (socket) socket->setKeepAliveIdleSecs(val);
    }

    /**
     * Return getKeepAliveIdleSecs() on underlying socket.
     */
    int getKeepAliveIdleSecs() const throw (nidas::util::IOException)
    {
	if (socket) return socket->getKeepAliveIdleSecs();
	return keepAliveIdleSecs;
    }

    /**
     * Return getInQueueSize() on underlying socket.
     */
    int getInQueueSize() const throw (nidas::util::IOException)
    {
	if (socket) return socket->getInQueueSize();
	return 0;
    }

    /**
     * Return getOutQueueSize() on underlying socket.
     */
    int getOutQueueSize() const throw (nidas::util::IOException)
    {
	if (socket) return socket->getOutQueueSize();
	return 0;
    }

    /**
     * Do the actual hardware read.
     */
    size_t read(void* buf, size_t len) throw (nidas::util::IOException);

    /**
    * Do the actual hardware write.
    */
    size_t write(const void* buf, size_t len) throw (nidas::util::IOException)
    {
	// std::cerr << "nidas::core::Socket::write, len=" << len << std::endl;
	return socket->send(buf,len,MSG_DONTWAIT | MSG_NOSIGNAL);
    }

    void close() throw (nidas::util::IOException)
    {
        if (socket) socket->close();
    }

    int getFd() const
    {
        if (socket) return socket->getFd();
	return -1;
    }

    const std::string& getName() const { return name; }

    void setName(const std::string& val) { name = val; }

    nidas::util::Inet4Address getRemoteInet4Address() const throw();

    /**
     * Create either a Socket or a McSocket from a DOMElement.
     */
    static IOChannel* createSocket(const xercesc::DOMElement*)
        throw(nidas::util::InvalidParameterException);

    void fromDOMElement(const xercesc::DOMElement*)
        throw(nidas::util::InvalidParameterException);

    xercesc::DOMElement*
        toDOMParent(xercesc::DOMElement* parent)
                throw(xercesc::DOMException);

    xercesc::DOMElement*
        toDOMElement(xercesc::DOMElement* node)
                throw(xercesc::DOMException);

protected:
    std::auto_ptr<nidas::util::SocketAddress> remoteSockAddr;

    nidas::util::Socket* socket;

    std::string name;

    bool firstRead;

    bool newFile;

    int keepAliveIdleSecs;
};

/**
 * Implementation of an IOChannel, over a ServerSocket.
 */
class ServerSocket: public IOChannel {

public:

    /**
     * Constructor.
     */
    ServerSocket(int port = 0);

    /**
     * Copy constructor.
     */
    ServerSocket(const ServerSocket& x);

    ~ServerSocket();

    ServerSocket* clone() const;

    void requestConnection(ConnectionRequester* service)
    	throw(nidas::util::IOException);

    IOChannel* connect() throw(nidas::util::IOException);

    const std::string& getName() const { return name; }

    void setName(const std::string& val) { name = val; }

    int getFd() const
    {
        if (servSock) return servSock->getFd();
	return -1;
    }


    /**
     * Set the value of keepAliveIdleSecs.  This is set on each
     * accepted socket connection. It does not pertain to the socket
     * which is waiting for connections.
     */
    void setKeepAliveIdleSecs(int val) throw (nidas::util::IOException)
    {
	keepAliveIdleSecs = val;
    }

    /**
     * Return keepAliveIdleSecs for this ServerSocket.
     */
    int getKeepAliveIdleSecs() const throw (nidas::util::IOException)
    {
	return keepAliveIdleSecs;
    }

    /**
    * ServerSocket will never be called to do an actual read.
    */
    size_t read(void* buf, size_t len) throw (nidas::util::IOException)
    {
	assert(false);
	return 0;
    }

    /**
    * ServerSocket will never be called to do an actual write.
    */
    size_t write(const void* buf, size_t len) throw (nidas::util::IOException)
    {
	assert(false);
	return 0;
    }

    void close() throw (nidas::util::IOException);

    /**
     * Create either a Socket or a McSocket from a DOMElement.
     */
    static IOChannel* createSocket(const xercesc::DOMElement*)
        throw(nidas::util::InvalidParameterException);

    void fromDOMElement(const xercesc::DOMElement*)
        throw(nidas::util::InvalidParameterException);

    xercesc::DOMElement*
        toDOMParent(xercesc::DOMElement* parent)
                throw(xercesc::DOMException);

    xercesc::DOMElement*
        toDOMElement(xercesc::DOMElement* node)
                throw(xercesc::DOMException);

protected:
    int port;

    std::string name;

    nidas::util::ServerSocket* servSock;

    ConnectionRequester* connectionRequester;

    nidas::util::Thread* thread;

    friend class ServerSocketConnectionThread;

    int keepAliveIdleSecs;

};

class ServerSocketConnectionThread: public nidas::util::Thread
{
public:
    ServerSocketConnectionThread(ServerSocket& sock):
    	Thread("ServerSocketConnectionThread"),socket(sock) {}

    int run() throw(nidas::util::IOException);

protected:
    ServerSocket& socket;
};

}}	// namespace nidas namespace core

#endif
