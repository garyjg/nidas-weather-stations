// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*-
// vim: set shiftwidth=4 softtabstop=4 expandtab:
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_CORE_SYNCRECORDSOURCE_H
#define NIDAS_CORE_SYNCRECORDSOURCE_H

#include <nidas/core/Resampler.h>
#include <nidas/core/SampleTag.h>

#include <vector>
#include <list>
#include <map>
#include <string>

#define SYNC_RECORD_ID 3
#define SYNC_RECORD_HEADER_ID 2

namespace nidas {

namespace core {
class Variable;
}

namespace dynld { namespace raf {

using namespace nidas::core;

class Aircraft;

class SyncRecordSource: public Resampler
{
public:
    
    SyncRecordSource();

    virtual ~SyncRecordSource();

    /**
     * This method and selectVariablesFromSensor() are used to select the
     * list of variables from a Project configuration in order of sensor,
     * with variables in order for each sensor, accepting only the
     * variables which make sense for Aircraft SyncRecords.  The variables
     * are appended to the @p variables list.
     *
     * Rather than rely on the sample tags from the source, this allows a
     * single function to be shared to accumulate processed sample tags and
     * variables directly from the Project.  Applications (like nimbus) can
     * use this to get the same list of Variables as would be retrieved from
     * SyncRecordReader, except they include all the metadata directly from
     * the Project instead of the sync record header.
     **/
    static void
    selectVariablesFromProject(Project* project, 
                               std::list<const Variable*>& variables);

    /**
     * See selectVariablesFromProject().
     **/
    static void
    selectVariablesFromSensor(DSMSensor* sensor, 
                              std::list<const Variable*>& variables);

    SampleSource* getRawSampleSource() { return 0; }

    SampleSource* getProcessedSampleSource() { return &_source; }

    /**
     * Get the output SampleTags.
     */
    std::list<const SampleTag*> getSampleTags() const
    {
        return _source.getSampleTags();
    }

    /**
     * Implementation of SampleSource::getSampleTagIterator().
     */
    SampleTagIterator getSampleTagIterator() const
    {
        return _source.getSampleTagIterator();
    }

    /**
     * Implementation of SampleSource::addSampleClient().
     */
    void addSampleClient(SampleClient* client) throw()
    {
        _source.addSampleClient(client);
    }

    void removeSampleClient(SampleClient* client) throw()
    {
        _source.removeSampleClient(client);
    }

    /**
     * Add a Client for a given SampleTag.
     * Implementation of SampleSource::addSampleClient().
     */
    void addSampleClientForTag(SampleClient* client,const SampleTag*) throw()
    {
        // I only have one tag, so just call addSampleClient()
        _source.addSampleClient(client);
    }

    void removeSampleClientForTag(SampleClient* client,const SampleTag*) throw()
    {
        _source.removeSampleClient(client);
    }

    int getClientCount() const throw()
    {
        return _source.getClientCount();
    }

    /**
     * Implementation of Resampler::flush().
     * Send current sync record, whether finished or not.
     */
    void flush() throw();

    const SampleStats& getSampleStats() const
    {
        return _source.getSampleStats();
    }

    void connect(SampleSource* source) throw();

    void disconnect(SampleSource* source) throw();

    void sendHeader(dsm_time_t timetag) throw();

    bool receive(const Sample*) throw();

protected:

    void addSensor(const DSMSensor* sensor) throw();

    void allocateRecord(dsm_time_t timetag);

    void init();

private:

    SampleSourceSupport _source;

    /**
     * Add a SampleTag to this SampleSource.
     */
    void addSampleTag(const SampleTag* tag) throw ()
    {
        _source.addSampleTag(tag);
    }

    void removeSampleTag(const SampleTag* tag) throw ()
    {
        _source.removeSampleTag(tag);
    }

    void createHeader(std::ostream&) throw();

    void
    pushSyncRecord(dsm_time_t tt);

    void
    preLoadCalibrations(dsm_time_t thead);

    int
    sampleIndexFromId(dsm_sample_id_t sampleId);

    /**
     * Construct all the sync record layout artifacts from the list of
     * variables set in _variables.  The layout includes settings like
     * variable lengths, sample indices, sample sizes and offsets, and
     * rates.
     *
     * SyncRecordSource first generates the list of variables with
     * selectVariablesFromProject() prior to calling layoutSyncRecord().
     **/
    void
    layoutSyncRecord();

    std::set<const DSMSensor*> _sensorSet;

    /**
     * A vector, with each element being a list of variables from a
     * sample. The order of elements in the vector is the order
     * of the variables in the sync record. By definition, every
     * variable in a sample has the same sampling rate.
     */
    std::vector<std::list<const Variable*> > _varsByIndex;

    /**
     * A mapping between sample ids and sample indices. Sample indices
     * range from 0 to the total number of different input samples -1.
     * When we receive a sample, what is its sampleIndex.
     */
    std::map<dsm_sample_id_t, int> _sampleIndices;

    /**
     * For each sample, by its index, the sampling rate, rounded up to an
     * integer.
     */
    std::vector<int> _intSamplesPerSec;

    /**
     * For each sample, the sampling rate, in floats.
     */
    std::vector<float> _rates;

    /**
     * For each sample, number of microseconds per sample,
     * 1000000/rate, truncated to an integer.
     */
    std::vector<int> _usecsPerSample;

    /**
     * For the first sample of each variable, its time offset
     * in microseconds into the second.
     */
    std::vector<int> _offsetUsec;

    /**
     * Number of values of each sample in the sync record.
     * This will be the rate * the number of values in each sample.
     */
    std::vector<size_t> _sampleLengths;

    /**
     * For each sample, its offset into the whole record.
     */
    std::vector<size_t> _sampleOffsets;

    /**
     * Offsets into the sync record of each variable in a sample,
     * indexed by sampleId.
     */
    std::vector<int*> _varOffsets;

    /**
     * Lengths of each variable in a sample,
     * indexed by sampleId.
     */
    std::vector<size_t*> _varLengths;

    /**
     * Number of variables in each sample.
     */
    std::vector<size_t> _numVars;

    /**
     * List of all variables in the sync record.
     */
    std::list<const Variable*> _variables;

    SampleTag _syncRecordHeaderSampleTag;

    SampleTag _syncRecordDataSampleTag;

    int _recSize;

    dsm_time_t _syncTime;

    SampleT<double>* _syncRecord;

    double* _dataPtr;

    size_t _unrecognizedSamples;

    std::ostringstream _headerStream;

    int _badLaterTimes;

    int _badEarlierTimes;

    const Aircraft* _aircraft;

    bool _initialized;

    int _unknownSampleType;

    /** No copying. */
    SyncRecordSource(const SyncRecordSource&);

    /** No assignment. */
    SyncRecordSource& operator=(const SyncRecordSource&);

};

}}}	// namespace nidas namespace dynld namespace raf

#endif
