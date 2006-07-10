
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_CORE_TESTSAMPLECLIENT_H
#define NIDAS_CORE_TESTSAMPLECLIENT_H

#include <nidas/core/SampleClient.h>

namespace nidas { namespace core {

/**
 * A little SampleClient for testing purposes.  Currently
 * just prints out some fields from the Samples it receives.
 */
class TestSampleClient : public SampleClient {
public:

  bool receive(const Sample *s) throw();

};

}}	// namespace nidas namespace core

#endif
