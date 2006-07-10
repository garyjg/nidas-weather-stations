/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate: 2005-12-02 12:24:18 -0700 (Fri, 02 Dec 2005) $

    $LastChangedRevision: 3171 $

    $LastChangedBy: maclean $

    $HeadURL: http://localhost:8080/svn/nids/trunk/dsm/class/PortSelector.cc $
 ********************************************************************

*/

#include <nidas/core/SamplePool.h>

using namespace nidas::core;
using namespace std;

namespace n_u = nidas::util;

/* static */
SamplePools* SamplePools::instance = 0;

/* static */
n_u::Mutex SamplePools::instanceLock = n_u::Mutex();

/* static */
SamplePools *SamplePools::getInstance()
{
    if (!instance) {
        n_u::Synchronized pooler(instanceLock);
        if (!instance) instance = new SamplePools();
    }
    return instance;
}

list<SamplePoolInterface*> SamplePools::getPools() const
{
    n_u::Synchronized pooler(instanceLock);
    return pools;
}

void SamplePools::addPool(SamplePoolInterface* pool)
{
    n_u::Synchronized pooler(instanceLock);
    pools.push_back(pool);
}

void SamplePools::removePool(SamplePoolInterface* pool)
{
    n_u::Synchronized pooler(instanceLock);
    list<SamplePoolInterface*>::iterator li =
	find(pools.begin(),pools.end(),pool);
    if (li != pools.end()) pools.erase(li);
}

