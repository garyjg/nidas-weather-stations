/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/

#ifndef NIDAS_DYNLD_STATISTICSPROCESSOR_H
#define NIDAS_DYNLD_STATISTICSPROCESSOR_H

#include <nidas/core/SampleIOProcessor.h>
#include <nidas/dynld/StatisticsCruncher.h>
#include <nidas/util/UTime.h>

namespace nidas { namespace dynld {

using namespace nidas::core;

/**
 * Interface of a processor of samples. A StatisticsProcessor reads
 * input Samples from a single SampleInput, and sends its processed
 * output Samples to one or more SampleOutputs.  
 */
class StatisticsProcessor: public SampleIOProcessor
{
public:

    StatisticsProcessor();

    ~StatisticsProcessor();

    /**
     * Request a sample from this StatisticsProcessor,
     * containing Parameters indicating what variables to
     * process and what kind of statistics to compute.
     */
    void addRequestedSampleTag(SampleTag* tag)
	throw(nidas::util::InvalidParameterException);

    /**
     * Do common operations necessary when a input has connected:
     * 1. Copy the DSMConfig information from the input to the
     *    disconnected outputs.
     * 2. Request connections for all disconnected outputs.
     *
     * connect() methods in subclasses should do whatever
     * initialization necessary before invoking this
     * StatisticsProcessor::connect().
     */
    void connect(SampleSource*) throw();

    /**
     * Disconnect a SampleInput from this StatisticsProcessor.
     * Right now just does a flush() of all connected outputs.
     */
    void disconnect(SampleSource*) throw();

    /**
     * Implementation of SampleConnectionRequester::connect.
     * Do common operations necessary when a output has connected:
     * 1. do: output->init().
     * 2. add output to a list of connected outputs.
     */
    void connect(SampleOutput* output) throw();

    /**
     * Implementation of SampleConnectionRequester::disconnect.
     * Do common operations necessary when a output has disconnected:
     * 1. do: output->close().
     * 2. remove output from a list of connected outputs.
     */
    void disconnect(SampleOutput* output) throw();

    void setStartTime(const nidas::util::UTime& val) 
    {
        _startTime = val;
    }

    nidas::util::UTime getStartTime() const
    {
        return _startTime;
    }

    void setEndTime(const nidas::util::UTime& val) 
    {
        _endTime = val;
    }

    nidas::util::UTime getEndTime() const
    {
        return _endTime;
    }

    float getPeriod() const 
    {
        return _statsPeriod;
    }

protected:

    /**
     * Implementation of SampleIOProcessor::addSampleTag(SampleTag*).
     */
    /*
    void addSampleTag(SampleTag* tag)
	    throw(nidas::util::InvalidParameterException);
    */

private:

    nidas::util::Mutex _connectionMutex;

    std::set<SampleSource*> _connectedSources;

    std::set<SampleOutput*> _connectedOutputs;


    std::list<StatisticsCruncher*> _crunchers;

    struct OutputInfo {
        StatisticsCruncher::statisticsType type;
	std::string countsName;
        bool higherMoments;
    };

    std::map<dsm_sample_id_t,struct OutputInfo> _infoBySampleId;

    nidas::util::UTime _startTime;

    nidas::util::UTime _endTime;

    float _statsPeriod;

    /**
     * Copy not supported
     */
    StatisticsProcessor(const StatisticsProcessor&);

    /**
     * Assignment not supported
     */
    StatisticsProcessor& operator=(const StatisticsProcessor&);

};

}}	// namespace nidas namespace core

#endif
