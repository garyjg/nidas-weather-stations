//
//              Copyright 2004 (C) by UCAR
//
// Description:
//

#include <nidas/util/Inet4SocketAddress.h>

#include <sstream>
#include <cstring>

using namespace nidas::util;
using namespace std;

Inet4SocketAddress::Inet4SocketAddress()
{
    memset(&sockaddr,0,sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
}

Inet4SocketAddress::Inet4SocketAddress(int port)
{
    memset(&sockaddr,0,sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;
}

Inet4SocketAddress::Inet4SocketAddress(const Inet4Address& addr, int port)
{
    memset(&sockaddr,0,sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr = *addr.getInAddrPtr();
}

Inet4SocketAddress::Inet4SocketAddress(const struct sockaddr_in* a):
	sockaddr(*a)
{
}

/* copy constructor */
Inet4SocketAddress::Inet4SocketAddress(const Inet4SocketAddress& x):
    sockaddr(x.sockaddr)
{
}

/* assignment operator */
Inet4SocketAddress& Inet4SocketAddress::operator=(const Inet4SocketAddress& x)
{
    if (this != &x) {
        sockaddr = x.sockaddr;
    }
    return *this;
}

/* clone */
Inet4SocketAddress* Inet4SocketAddress::clone() const
{
    return new Inet4SocketAddress(*this);
}

std::string Inet4SocketAddress::toString() const
{
    std::ostringstream ost;
    ost << "inet:" << getInet4Address().getHostName() <<
	    ':' << getPort();
    return ost.str();
}

std::string Inet4SocketAddress::toAddressString() const
{
    std::ostringstream ost;
    ost << "inet:" << getInet4Address().getHostAddress() <<
	    ':' << getPort();
    return ost.str();
}

/**
 * Comparator operator for addresses. Useful if this
 * address is a key in an STL map.
 */
bool Inet4SocketAddress::operator < (const Inet4SocketAddress& x) const {
    if(ntohl(sockaddr.sin_addr.s_addr) <
	    ntohl(x.sockaddr.sin_addr.s_addr)) return true;
    if (sockaddr.sin_addr.s_addr == x.sockaddr.sin_addr.s_addr)
	return ntohs(sockaddr.sin_port) < ntohs(x.sockaddr.sin_port);
    return false;
}

/**
 * Equality operator for addresses.
 */
bool Inet4SocketAddress::operator == (const Inet4SocketAddress& x) const {
    return sockaddr.sin_addr.s_addr == x.sockaddr.sin_addr.s_addr &&
	sockaddr.sin_port == x.sockaddr.sin_port;
}
