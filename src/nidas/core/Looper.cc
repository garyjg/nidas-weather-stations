/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/

#include <nidas/core/Looper.h>
#include <nidas/util/Logger.h>

using namespace std;
using namespace nidas::core;

namespace n_u = nidas::util;

/* static */
Looper* Looper::instance = 0;

/* static */
n_u::Mutex Looper::instanceMutex;

/* static */
Looper* Looper::getInstance()
{
    if (!instance) {
	n_u::Synchronized autosync(instanceMutex);
	if (!instance) instance = new Looper();
    }
    return instance;
}

/* static */
void Looper::removeInstance()
{
    instanceMutex.lock();
    delete instance;
    instance = 0;
    instanceMutex.unlock();
}
Looper::Looper(): n_u::Thread("Looper"),sleepMsec(0)
{
}

void Looper::addClient(LooperClient* clnt, unsigned int msecPeriod)
	throw(n_u::InvalidParameterException)
{

    if (msecPeriod < 5) throw n_u::InvalidParameterException(
    	"Looper","addClient","requested callback period is too small ");
    // round to nearest 10 milliseconds, which better
    // matches the precision of the system nanosleep.
    msecPeriod += 5;
    msecPeriod = msecPeriod - msecPeriod % 10;

    n_u::Synchronized autoLock(clientMutex);

    map<unsigned int,std::set<LooperClient*> >::iterator
    	ci = clientsByPeriod.find(msecPeriod);

    if (ci != clientsByPeriod.end()) ci->second.insert(clnt);
    else {
	/* new period value */
        set<LooperClient*> clnts;
	clnts.insert(clnt);
	clientsByPeriod[msecPeriod] = clnts;
    }
    setupClientMaps();
}

void Looper::removeClient(LooperClient* clnt)
{
    clientMutex.lock();

    map<unsigned int,std::set<LooperClient*> >::iterator
    	ci = clientsByPeriod.begin();

    bool foundClient = false;
    for ( ; ci != clientsByPeriod.end(); ++ci) {
        set<LooperClient*>::iterator si = ci->second.find(clnt);
	if (si != ci->second.end()) {
	    ci->second.erase(si);
	    foundClient = true;
	}
    }
    if (foundClient) setupClientMaps();
    bool haveClients = cntrMods.size() > 0;
    clientMutex.unlock();

    if (!haveClients && isRunning()) {
	interrupt();
	if (Thread::currentThreadId() != getId()) {
	    n_u::Logger::getInstance()->log(LOG_INFO,
		"Interrupted Looper, doing join");
	    join();
	}
    }
}

/* Use Euclidian resursive algorimthm to find greatest common divisor.
 * Thanks to Wikipedia. */
int Looper::gcd(unsigned int a, unsigned int b)
{
    if (b == 0) return a;
    return gcd(b,a % b);
}

void Looper::setupClientMaps()
{
    map<unsigned int,std::set<LooperClient*> >::iterator ci;
    unsigned int sleepval = 0;

    /* determine greatest common divisor of periods */
    for (ci = clientsByPeriod.begin(); ci != clientsByPeriod.end(); ) {
        if (ci->second.size() == 0) clientsByPeriod.erase(ci++);
	else {
	    unsigned int per = ci->first;
	    if (sleepval == 0) sleepval = per;
	    else sleepval = gcd(sleepval,per);
	    ++ci;
	}
    }

    clientsByCntrMod.clear();
    cntrMods.clear();
    if (sleepval == 0) return;      // no clients

    for (ci = clientsByPeriod.begin(); ci != clientsByPeriod.end(); ++ci) {
	assert (ci->second.size() > 0);
	unsigned int per = ci->first;
	assert((per % sleepval) == 0);
	int cntrMod = per / sleepval;
	list<LooperClient*> clnts(ci->second.begin(),ci->second.end());
	clientsByCntrMod[cntrMod] = clnts;
	cntrMods.insert(cntrMod);
    }
    sleepMsec = sleepval;
    n_u::Logger::getInstance()->log(LOG_INFO,
	"Looper, sleepMsec=%d",sleepMsec);

    if (!isRunning()) start();
}

/* static */
bool Looper::sleepUntil(unsigned int periodMsec,unsigned int offsetMsec)
	throw(n_u::IOException)
{
    struct timespec sleepTime;
    /*
     * sleep until an even number of periodMsec since 
     * creation of the universe (Jan 1, 1970 0Z).
     */
    dsm_time_t tnow = getSystemTime();
    unsigned int mSecVal =
      periodMsec - (unsigned int)((tnow / USECS_PER_MSEC) % periodMsec) + offsetMsec;

    sleepTime.tv_sec = mSecVal / MSECS_PER_SEC;
    sleepTime.tv_nsec = (mSecVal % MSECS_PER_SEC) * NSECS_PER_MSEC;
    if (::nanosleep(&sleepTime,0) < 0) {
	if (errno == EINTR) return true;
	throw n_u::IOException("Looper","nanosleep",errno);
    }
    return false;
}

/**
 * Thread function, the loop.
 */

int Looper::run() throw(n_u::Exception)
{
    if (sleepUntil(sleepMsec)) return RUN_OK;

    n_u::Logger::getInstance()->log(LOG_INFO,
    	"Looper starting, sleepMsec=%d", sleepMsec);
    while (!amInterrupted()) {
	dsm_time_t tnow = getSystemTime() / USECS_PER_MSEC;
	unsigned int cntr = (unsigned int)(tnow % MSECS_PER_DAY) / sleepMsec;

	clientMutex.lock();
	// make a copy of the list
	list<int> mods(cntrMods.begin(),cntrMods.end());
	clientMutex.unlock();

	list<int>::const_iterator mi = mods.begin();
	for ( ; mi != mods.end(); ++mi) {
	    int modval = *mi;

	    if (!(cntr % modval)) {
		clientMutex.lock();
		// make a copy of the list
		list<LooperClient*> clients = clientsByCntrMod[modval];
		clientMutex.unlock();

		list<LooperClient*>::const_iterator li = clients.begin();
		for ( ; li != clients.end(); ++li) {
		    LooperClient* clnt = *li;
		    clnt->looperNotify();
		}
	    }
	}
	if (sleepUntil(sleepMsec)) return RUN_OK;
    }
    return RUN_OK;
}


