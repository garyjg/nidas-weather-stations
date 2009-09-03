/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/


#ifndef NIDAS_DYNLD_XMLCONFIGSERVICE_H
#define NIDAS_DYNLD_XMLCONFIGSERVICE_H

#include <nidas/core/DSMService.h>
#include <nidas/core/IOChannel.h>

namespace nidas { namespace dynld {

using namespace nidas::core;

class XMLConfigService: public DSMService, public IOChannelRequester
{
public:
    XMLConfigService();

    ~XMLConfigService();

    void interrupt() throw();

    void connected(IOChannel*) throw();

    void connect(SampleInput*) throw() { assert(false); }
    void connect(SampleOutput*) throw() { assert(false); }
    void disconnect(SampleInput*) throw() { assert(false); }
    void disconnect(SampleOutput*) throw() { assert(false); }

    void schedule() throw(nidas::util::Exception);

    void fromDOMElement(const xercesc::DOMElement* node)
	throw(nidas::util::InvalidParameterException);

private:

    /**
     * Worker thread that is run when a connection comes in.
     */
    class Worker: public nidas::util::Thread
    {
        public:
            Worker(XMLConfigService* svc,IOChannel* iochan,const DSMConfig* dsm);
            ~Worker();
            int run() throw(nidas::util::Exception);
        private:
            XMLConfigService* _svc;
            IOChannel* _iochan;
            const DSMConfig* _dsm;
    };

    /**
     * Copying not supported.
     */
    XMLConfigService(const XMLConfigService&);

    /**
     * Assignment not supported.
     */
    XMLConfigService& operator =(const XMLConfigService&);

};

}}	// namespace nidas namespace core

#endif
