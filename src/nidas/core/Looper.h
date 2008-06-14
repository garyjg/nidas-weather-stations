
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/


#ifndef NIDAS_CORE_LOOPER_H
#define NIDAS_CORE_LOOPER_H

#include <nidas/core/DSMTime.h>
#include <nidas/core/LooperClient.h>

#include <nidas/util/Thread.h>
#include <nidas/util/ThreadSupport.h>
#include <nidas/util/IOException.h>
#include <nidas/util/InvalidParameterException.h>

#include <sys/time.h>
#include <sys/select.h>

namespace nidas { namespace core {

/**
 * Looper is a Thread that periodically loops, calling
 * the LooperClient::looperNotify() method of
 * LooperClients at the requested intervals.
 */
class Looper : public nidas::util::Thread {
public:

    /**
     * Fetch the pointer to the instance of Looper
     */
    static Looper* getInstance();

    /**
     * Delete the instance of Looper. Probably only done
     * at main program shutdown, and then only if you're really
     * worried about cleaning up.
     */
    static void removeInstance();

    /**
     * Add a client to the Looper whose
     * LooperClient::looperNotify() method should be
     * called every msec number of milliseconds.
     * @param msecPeriod Time period, in milliseconds.
     * Since the system nanosleep function is only precise
     * to about 10 milliseconds, and to reduce system load,
     * this value is rounded to the nearest 10 milliseconds.
     */
    void addClient(LooperClient *clnt,unsigned int msecPeriod)
    	throw(nidas::util::InvalidParameterException);

    /**
     * Remove a client from the Looper.
     */
    void removeClient(LooperClient *clnt);

    /**
     * Thread function.
     */
    virtual int run() throw(nidas::util::Exception);

    /**
     * Utility function, sleeps until the next even period + offset.
     */
    static bool sleepUntil(unsigned int periodMsec,unsigned int offsetMsec=0)
    	throw(nidas::util::IOException);

    /**
     * Utility function for finding greatest common divisor.
     */
    static int gcd(unsigned int a, unsigned int b);

private:

    void setupClientMaps();

    /**
     * Constructor.
     */
    Looper();

    static Looper* instance;

    static nidas::util::Mutex instanceMutex;

    nidas::util::Mutex clientMutex;

    std::map<unsigned int,std::set<LooperClient*> > clientsByPeriod;

    std::map<int,std::list<LooperClient*> > clientsByCntrMod;

    std::set<int> cntrMods;

    unsigned int sleepMsec;
};

}}	// namespace nidas namespace core

#endif
