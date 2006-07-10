
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$


*/

#ifndef NIDAS_CORE_SAMPLECLIENTLIST_H
#define NIDAS_CORE_SAMPLECLIENTLIST_H

#include <list>

#include <nidas/util/ThreadSupport.h>
#include <nidas/core/SampleClient.h>

namespace nidas { namespace core {

/**
 * A list of SampleClients.  This class is typically used by
 * SampleSources to maintain their client lists.
 */
class SampleClientList {
public:

  SampleClientList() {}

  /**
   * Public copy constructor. To support multithreading,
   * this first locks the SampleClientList argument.
   */
  SampleClientList(const SampleClientList& cl);

  virtual ~SampleClientList() {}

  /**
   * Public assignment operator.  To support multithreading,
   * this first locks the SampleClientList argument.
   */
  SampleClientList& operator=(const SampleClientList& cl);

  /**
   * Add a SampleClient to this list.  The pointer
   * to the SampleClient must remain valid, until after
   * it is removed. A nidas::util::Mutex exclusion lock
   * is used to avoid simultaneous access.
   */
  virtual void add(SampleClient*);

  /**
   * Remove a SampleClient from this SampleSource.
   * A nidas::util::Mutex exclusion lock
   * is used to avoid simultaneous access.
   */
  virtual void remove(SampleClient*);

  /**
   * How many clients?
   */
  virtual int size() const;

  /**
   * Big cleanup.
   * A nidas::util::Mutex exclusion lock
   * is used to avoid simultaneous access.
   */
  virtual void removeAll();

  /**
   * Lock this list.
   */
  void lock() const throw() { clistLock.lock(); }

  /**
   * Unlock this list.
   */
  void unlock() const throw() { clistLock.unlock(); }

  /** get a const_iterator pointing to first element. Does not lock! */
  std::list<SampleClient*>::const_iterator begin() throw() { return clients.begin(); }

  /** get a const_iterator pointing to one-past-last element. Does not lock! */
  std::list<SampleClient*>::const_iterator end() throw() { return clients.end(); }

protected:

  /**
   * mutex to prevent simultaneous access to clients list
   */
  mutable nidas::util::Mutex clistLock;

  /**
   * My current clients.
   */
  std::list<SampleClient*> clients;

};

}}	// namespace nidas namespace core

#endif
