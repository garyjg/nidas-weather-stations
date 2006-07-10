//
//              Copyright 2004 (C) by UCAR
//
// Description:
//

#include <nidas/util/Inet4Address.h>

#include <netdb.h>
#include <cctype>	// isdigit

// #define DEBUG
#include <iostream>
#include <cassert>

using namespace nidas::util;
using namespace std;

/* static */
std::map<Inet4Address,std::string> Inet4Address::addrToName;

/* static */
Mutex Inet4Address::addrToNameLock;

/* static */
std::map<std::string,std::list<Inet4Address> > Inet4Address::nameToAddrs;

/* static */
Mutex Inet4Address::nameToAddrsLock;

Inet4Address::Inet4Address()
{
    memset(&inaddr,0,sizeof(inaddr));
}

Inet4Address::Inet4Address(const struct in_addr* a):
	inaddr(*a)
{
}

Inet4Address::Inet4Address(unsigned int a)
{
	inaddr.s_addr = a;
}

string Inet4Address::getHostAddress() const {
    char caddr[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET,&inaddr,caddr,sizeof caddr))
	return "inet_ntop error";	// unlikely
    return caddr;
}

const string Inet4Address::getHostName() const throw() {
  return getHostName(*this);
}

/* static */
const string Inet4Address::getHostName(const Inet4Address& addr) throw()
{
    {
	Synchronized sync(addrToNameLock);
	map<Inet4Address,string>::iterator ai =
	    addrToName.find(addr);
	if (ai != addrToName.end()) {
#ifdef DEBUG
	    cerr << "getHostName: " << addr.getHostAddress() <<
	    	" name=" << ai->second << " cached" << endl;
#endif
	    return ai->second;
	}
    }

    if (addr.inaddr.s_addr == INADDR_ANY) {
	pair<Inet4Address,string> p1;
	Synchronized sync(addrToNameLock);

	p1.first = addr;
	p1.second = addr.getHostAddress();
	addrToName.insert(p1);
	return p1.second;
    }
#ifdef DEBUG
    cerr << "getHostName: " << addr.getHostAddress() <<
    	" is not in cache" << endl;
#endif

    struct hostent hent;
    struct hostent *result;
    int h_error = TRY_AGAIN;
    const int numtries = 5;
    char auxbuf[1024];	// haven't found any doc about how big this should be

    for (int ntry=0; h_error == TRY_AGAIN ; ntry++) {
	if (!gethostbyaddr_r(addr.getInAddrPtr(),sizeof(struct in_addr),
		AF_INET, &hent,auxbuf,sizeof(auxbuf),
		&result,&h_error)) {
	    h_error = NETDB_SUCCESS;
	    break;	// success
	}
        switch(h_error) {
	case NETDB_SUCCESS:
#ifdef DEBUG
	    cerr << "gethostbyaddr_r NETDB_SUCCESS" << endl;
#endif
	    break;
	case HOST_NOT_FOUND:
#ifdef DEBUG
	    cerr << "gethostbyaddr_r HOST_NOT_FOUND" << endl;
#endif
	    break;
	case NO_ADDRESS:		// same as NO_DATA
#ifdef DEBUG
	    cerr << "gethostbyaddr_r NO_ADDRESS" << endl;
#endif
	    // throw UnknownHostException(addr.getHostAddress());
	    break;
	case NO_RECOVERY:
#ifdef DEBUG
	    cerr << "gethostbyaddr_r NO_RECOVERY" << endl;
#endif
	    // throw UnknownHostException(addr.getHostAddress() + ": nameserver error");
	    break;
	case TRY_AGAIN:
#ifdef DEBUG
	    cerr << "gethostbyaddr_r TRY_AGAIN, ntry=" << ntry << endl;
#endif
	    if (ntry == numtries) {
		// throw UnknownHostException(addr.getHostAddress() +
		// 	": numerous temporary name server errors");

	        h_error = NO_RECOVERY;
	    }
	    else {
		struct timespec slp;
		slp.tv_sec = 1;
		slp.tv_nsec = 0;
		nanosleep(&slp,0);
	    }
	    break;
	default:
#ifdef DEBUG
#endif
	    cerr << "gethostbyaddr_r unknown error=" << h_error  << endl;
	    assert(0);
	}
    }
#ifdef DEBUG
    cerr << "result=" << hex << (void*)result << " &hent=" <<
    	(void*)&hent << dec << " addr=" << addr.getHostAddress() << endl;
    cerr << "hent.h_name=" << hex << (void*) hent.h_name << 
	" strlen(hent.h_name)=" << strlen(hent.h_name) << endl;
#endif

    pair<Inet4Address,string> p1;
    p1.first = addr;

    // result will point to hent on success
    if (h_error == NETDB_SUCCESS && result == &hent)
    	p1.second = string(hent.h_name);
    else p1.second = addr.getHostAddress();

    addrToNameLock.lock();
    addrToName.insert(p1);
    addrToNameLock.unlock();

    return p1.second;
}

/* static */
list<Inet4Address> Inet4Address::getAllByName(const string& hostname)
  	throw(UnknownHostException)
{
    {
	Synchronized sync(nameToAddrsLock);
	map<string,list<Inet4Address> >::iterator ai =
	    nameToAddrs.find(hostname);
	if (ai != nameToAddrs.end()) {
#ifdef DEBUG
	    cerr << "getAllByName: " << hostname << " addr=" <<
	    	ai->second.front().getHostAddress() << " cached" << endl;
#endif
	    return ai->second;
	}
    }
#ifdef DEBUG
    cerr << "getAllByName: " << hostname << " is not in cache" << endl;
#endif

    struct hostent hent;
    struct hostent *result;
    int h_error;
    const int numtries = 5;
    char auxbuf[1024];	// haven't found any doc about how big this should be

    for (int ntry=0; ; ntry++) {
	if (!gethostbyname_r(hostname.c_str(),&hent,
		auxbuf,sizeof(auxbuf),&result,&h_error)) break;	// success
        switch(h_error) {
	case HOST_NOT_FOUND:
	case NO_ADDRESS:		// same as NO_DATA
	    throw UnknownHostException(hostname);
	case NO_RECOVERY:
	    throw UnknownHostException(hostname + ": nameserver error");
	case TRY_AGAIN:
#ifdef DEBUG
	    cerr << "gethostbyaddr_r TRY_AGAIN, ntry=" << ntry << endl;
#endif
	    if (ntry == numtries)
		throw UnknownHostException(hostname +
			": numerous temporary name server errors");
	    {
		struct timespec slp;
		slp.tv_sec = 1;
		slp.tv_nsec = 0;
		nanosleep(&slp,0);
	    }
	    break;
	default:
#ifdef DEBUG
#endif
	    cerr << "gethostbyname_r unknown error=" << h_error  << endl;
	    assert(0);
	}
    }
    if (!result) throw UnknownHostException(hostname);

    assert(result == &hent);
    assert(hent.h_length == sizeof(struct in_addr));

    list<Inet4Address> addrlist;
    for (int i = 0; hent.h_addr_list[i]; i++) {
        Inet4Address iaddr((struct in_addr*)hent.h_addr_list[i]);
	addrlist.push_back(iaddr);
    }

    {
	pair<string,list<Inet4Address> > p1;
	Synchronized autolock(nameToAddrsLock);

	p1.first = hostname;
	p1.second = addrlist;
	nameToAddrs.insert(p1);

	// add entry with official name
	p1.first = string(hent.h_name);
	p1.second = addrlist;
	nameToAddrs.insert(p1);
    }

#ifdef DONT_DO_THIS
    // add reverse lookup with given hostname
    if (addrlist.size() > 0) {
	pair<Inet4Address,string> p2;
	p2.first = addrlist.front();
	p2.second = hostname;
	Synchronized autolock(addrToNameLock);
	addrToName.insert(p2);
    }
#endif

    return addrlist;
}

/* static */
Inet4Address Inet4Address::getByName(const string& hostname)
  	throw(UnknownHostException)
{
    list<Inet4Address> addrs = getAllByName(hostname);
    if (addrs.size() > 0) return addrs.front();
    else throw UnknownHostException(hostname);
}

/*
 * How many leading bits match in the two addresses.
 * We compute it by taking the xor and shifting
 * right until it is zero.
 */
int Inet4Address::bitsMatch(const Inet4Address& x) const throw()
{
    unsigned long addr1 = ntohl(inaddr.s_addr);
    unsigned long addr2 = ntohl(x.inaddr.s_addr);
    unsigned long match = addr1 ^ addr2;
    int i;
    for (i = 32; match > 0 ; i--) match >>= 1;
    return i;
}
