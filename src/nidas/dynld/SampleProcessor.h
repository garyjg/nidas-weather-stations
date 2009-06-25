/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate: 2007-01-31 11:23:38 -0700 (Wed, 31 Jan 2007) $

    $LastChangedRevision: 3648 $

    $LastChangedBy: cjw $

    $HeadURL: http://svn.eol.ucar.edu/svn/nidas/trunk/src/nidas/dynld/ProcessedSampleUDPService.h $
 ********************************************************************

*/


#ifndef NIDAS_DYNLD_PROCESSEDSAMPLEUDPSERVICE_H
#define NIDAS_DYNLD_PROCESSEDSAMPLEUDPSERVICE_H

#include <nidas/core/DSMService.h>
#include <nidas/core/DatagramSocket.h>

namespace nidas { namespace dynld {

using namespace nidas::core;

class SampleProcessor: public SampleIOProcessor
{
public:
    SampleProcessor();

    ~SampleProcessor();

    void connect(SampleInput*) throw();

    void disconnect(SampleInput*) throw();

    void connect(SampleOutput* orig,SampleOutput* output) throw();

    void disconnect(SampleOutput* output) throw();


private:

    SampleInput* _input;

    std::list<SampleInput*> _connectedInputs;

    /**
     * Copy not supported.
     */
    SampleProcessor(const SampleProcessor&);

    /**
     * Assignment not supported.
     */
    SampleProcessor& operator=(const SampleProcessor&);

};


}}	// namespace nidas namespace core

#endif
