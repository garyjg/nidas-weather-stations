
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_DYNLD_SAMPLEARCHIVER_H
#define NIDAS_DYNLD_SAMPLEARCHIVER_H

#include <nidas/core/SampleIOProcessor.h>
#include <nidas/core/SampleSorter.h>
#include <nidas/dynld/FileSet.h>
#include <nidas/util/ThreadSupport.h>

namespace nidas { namespace dynld {

using namespace nidas::core;

class SampleArchiver: public SampleIOProcessor
{
public:
    
    SampleArchiver();

    virtual ~SampleArchiver();

    // SampleArchiver* clone() const;

    // bool cloneOnConnection() const { return false; }

    void connect(SampleInput*) throw();

    void disconnect(SampleInput*) throw();

    void connect(SampleOutput* orig,SampleOutput* output) throw();

    void disconnect(SampleOutput* output) throw();

    void printStatus(std::ostream&,float deltat,int&) throw();

protected:

    SampleInput* _input;

    /**
     * If my SampleOutput* is a nidas::dynld::FileSet then save the pointer
     * for use in by printStatus(), so that the status output will contain
     * things like the file size.
     */
    std::list<const nidas::dynld::FileSet*> _filesets;

    /**
     * Mutex for controlling access to _input and _fileset
     * so that printStatus has valid pointers.
     */
    nidas::util::Mutex _statusMutex;

    /**
     * Saved between calls to printStatus in order to compute sample rates.
     */
    size_t _nsampsLast;

    /**
     * Saved between calls to printStatus in order to compute data rates.
     */
    long long _nbytesLast;

    std::map<const nidas::dynld::FileSet*,long long> _nbytesLastByFileSet;

private:
    /**
     * Copy not supported.
     */
    SampleArchiver(const SampleArchiver& x);

    /**
     * Assignment not supported.
     */
    SampleArchiver& operator=(const SampleArchiver& x);
};

}}	// namespace nidas namespace core

#endif
