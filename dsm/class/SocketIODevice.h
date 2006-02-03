/*
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate: 2006-01-20 09:58:25 -0700 (Fri, 20 Jan 2006) $

    $LastChangedRevision: 3245 $

    $LastChangedBy: maclean $

    $HeadURL: http://svn/svn/nids/trunk/dsm/class/SocketIODevice.h $

*/
#ifndef SOCKETIODEVICE_H
#define SOCKETIODEVICE_H

#include <IODevice.h>
#include <atdUtil/Socket.h>
#include <atdUtil/ParseException.h>

namespace dsm {

/**
 * A sensor connected through a socket.
 */
class SocketIODevice : public IODevice {

public:

    /**
     * Create a SocketIODevice.  No IO operations to the sensor
     * are performed in the constructor (hence no IOExceptions).
     */
    SocketIODevice();

    virtual ~SocketIODevice();

    /**
     * The file descriptor used when reading from this SocketIODevice.
     */
    int getReadFd() const
    {
	if (socket) return socket->getFd();
	return -1;
    }

    /**
     * The file descriptor used when writing to this sensor.
     */
    int getWriteFd() const {
	if (socket) return socket->getFd();
    	return -1;
    }

    /**
    * open the socket.
    */
    void open(int flags)
    	throw(atdUtil::IOException,atdUtil::InvalidParameterException);

    /**
    * Read from the sensor.
    */
    size_t read(void *buf, size_t len) throw(atdUtil::IOException)
    {
        return socket->recv(buf,len);
    }

    /**
    * Write to the sensor.
    */
    size_t write(const void *buf, size_t len) throw(atdUtil::IOException) 
    {
        return socket->send(buf,len);
    }

    /*
    * Perform an ioctl on the device. Not necessary for a socket,
    * and will throw an IOException.
    */
    void ioctl(int request, void* buf, size_t len) throw(atdUtil::IOException)
    {
        throw atdUtil::IOException(getName(),
		"ioctl","not supported on SocketIODevice");
    }

    /**
    * close the sensor (and any associated FIFOs).
    */
    void close() throw(atdUtil::IOException);

    void setTcpNoDelay(bool val) throw(atdUtil::IOException)
    {
        tcpNoDelay = val;
    }

    bool getTcpNoDelay() throw(atdUtil::IOException)
    {
	return tcpNoDelay;
    }

    void parseAddress(const std::string& name) throw(atdUtil::ParseException);

protected:

    /**
     * The type of the destination address, AF_INET or AF_UNIX.
     */
    int addrtype;	

    /**
     * Destination host name from sensor name.
     */
    std::string desthost;

    /**
     * Port number that is parsed from sensor name.
     */
    int destport;

    /**
     * The destination socket address.
     */
    std::auto_ptr<atdUtil::SocketAddress> sockAddr;

    /**
     * The socket.  This isn't in an auto_ptr because
     * one must close the socket prior to deleting it.
     * The atdUtil::Socket destructor does not close
     * the file descriptor.
     */
    atdUtil::Socket* socket;

    bool tcpNoDelay;

};

}
#endif
