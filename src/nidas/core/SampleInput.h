/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/

#ifndef NIDAS_CORE_SAMPLEINPUT_H
#define NIDAS_CORE_SAMPLEINPUT_H


#include <nidas/core/SampleSource.h>
#include <nidas/core/IOStream.h>
#include <nidas/core/SampleSorter.h>
#include <nidas/core/SampleInputHeader.h>

namespace nidas { namespace core {

class DSMConfig;
class DSMSensor;

/**
 * Interface of an input SampleSource. Typically a SampleInput is
 * reading serialized samples from a socket or file, and
 * then sending them on.
 *
 */
class SampleInput: public SampleSource
{
public:

    virtual ~SampleInput() {}

    virtual std::string getName() const = 0;

    // virtual nidas::util::Inet4Address getRemoteInet4Address() const = 0;
    virtual const DSMConfig* getDSMConfig() const = 0;

    /**
     * Client wants samples from the process() method of the
     * given DSMSensor.
     */
    virtual void addProcessedSampleClient(SampleClient*,DSMSensor*) = 0;

    virtual void removeProcessedSampleClient(SampleClient*,DSMSensor* = 0) = 0;

    /**
     * Set length of SampleSorter, in milliseconds.
     */
    virtual void setSorterLengthMsecs(int val) = 0;

    virtual int getSorterLengthMsecs() const = 0;

    /**
     * Set the maximum amount of heap memory to use for sorting samples.
     * @param val Maximum size of heap in bytes.
     * @see SampleSorter::setHeapMax().
     */
    virtual void setHeapMax(size_t val) = 0;

    virtual size_t getHeapMax() const = 0;

    /**
     * @param val If true, and heapSize exceeds heapMax,
     *   then wait for heapSize to be less then heapMax,
     *   which will block any SampleSources that are inserting
     *   samples into this sorter.  If false, then discard any
     *   samples that are received while heapSize exceeds heapMax.
     * @see SampleSorter::setHeapBlock().
     */
    virtual void setHeapBlock(bool val) = 0;

    virtual bool getHeapBlock() const = 0;

};

/**
 * SampleInputMerger sorts samples that are coming from one
 * or more inputs.  Samples can then be passed onto sensors
 * for processing, and then sorted again.
 *
 * SampleInputMerger makes use of two SampleSorters, _inputSorter
 * and procSampleSorter.
 *
 * _inputSorter is a client of one or more inputs (the text arrows
 * show the sample flow):
 *
 * input ----v
 * input --> _inputSorter
 * input ----^
 *
 * After sorting the samples, _inputSorter passes them onto the two
 * types of SampleClients that have registered with SampleInputMerger.
 * SampleClients that have registered with
 * SampleInputMerger::addSampleClient will receive their raw samples
 * directly from _inputSorter.
 *
 * _inputSorter -> sampleClients
 *
 * SampleClients that have registered with
 * SampleInputMerger::addProcessedSampleClient will receive their
 * samples indirectly:
 * 
 * _inputSorter -> this -> sensor -> procSampSorter -> processedSampleClients
 *
 * _inputSorter provides sorting of the samples from the various inputs.
 *
 * procSampSorter provides sorting of the processed samples.
 * Sensors are apt to create processed samples with different
 * time-tags than the input raw samples, therefore they need
 * to be sorted again.
 */
class SampleInputMerger: public SampleInput , protected SampleClient
{
public:
    SampleInputMerger();

    virtual ~SampleInputMerger();

    std::string getName() const { return _name; }

    const DSMConfig* getDSMConfig() const { return 0; }

    /**
     * Add an input to be merged and sorted.
     */
    void addInput(SampleInput* input);

    void removeInput(SampleInput* input);

    /**
     * Add a SampleClient that wants samples which have been
     * merged from various inputs, sorted, processed through a
     * certain DSMSensor, and then re-sorted again.
     */
    void addProcessedSampleClient(SampleClient*,DSMSensor*);

    void removeProcessedSampleClient(SampleClient*,DSMSensor* = 0);

    /**
     * Add a SampleClient that wants samples which have been
     * merged from various inputs and then sorted.
     */
    void addSampleClient(SampleClient* client) throw();

    void removeSampleClient(SampleClient* client) throw();

    bool receive(const Sample*) throw();

    void finish() throw();

    void setRealTime(bool val) 
    {
        _inputSorter.setRealTime(val);
    }

    bool getRealTime() const
    {
        return _inputSorter.getRealTime();
    }

    /**
     * Add a SampleTag to this merger. SampleInputMerger does not
     * own the pointer.
     */
    void addSampleTag(const SampleTag* stag);

    const std::list<const SampleTag*>& getSampleTags() const
    {
        return _sampleTags;
    }

    /**
     * What DSMConfigs are associated with this SampleInput.
     */
    const std::list<const DSMConfig*>& getDSMConfigs() const
    {
        return _dsmConfigs;
    }

    /**
     * Total number of bytes received.
     */
    long long getNumReceivedBytes() const
    {
        return _inputSorter.getNumReceivedBytes();
    }

    /**
     * Total number of samples received.
     */
    long long getNumReceivedSamples() const
    {
        return _inputSorter.getNumReceivedSamples();
    }

    /**
     * Timetag of most recent sample inserted into the merger.
     */
    dsm_time_t getLastReceivedTimeTag() const
    {
        return _inputSorter.getLastReceivedTimeTag();
    }

    /**
     * Number of samples distributed to my clients.
     */
    size_t getNumDistributedSamples() const
    {
        return _inputSorter.getNumDistributedSamples();
    }

    /**
     * Number of bytes distributed to my clients.
     */
    long long getNumDistributedBytes() const
    {
        return _inputSorter.getNumDistributedBytes();
    }

    /**
     * Timetag of most recent sample distributed by the merger.
     */
    dsm_time_t getLastDistributedTimeTag() const
    {
        return _inputSorter.getLastDistributedTimeTag();
    }

    /**
     * Number of samples currently in the sorter.
     */
    size_t getSorterNumSamples() const
    {
        return _inputSorter.size();
    }

    /**
     * Current size in bytes of the sorter.
     */
    size_t getSorterNumBytes() const
    {
        return _inputSorter.getHeapSize();
    }

    /**
     * Current size in bytes of the sorter.
     */
    size_t getSorterNumBytesMax() const
    {
        return _inputSorter.getHeapMax();
    }

    /**
     * Number of samples discarded because sorter was getting
     * too big.
     */
    size_t getNumDiscardedSamples() const
    {
        return _inputSorter.getNumDiscardedSamples();
    }

    /**
     * Number of samples discarded because their timetags 
     * were in the future.
     */
    size_t getNumFutureSamples() const
    {
        return _inputSorter.getNumFutureSamples();
    }

    /**
     * Set length of SampleSorter, in milliseconds.
     */
    void setSorterLengthMsecs(int val)
    {
        _sorterLengthMsecs = val;
    }

    int getSorterLengthMsecs() const
    {
        return _sorterLengthMsecs;
    }

    /**
     * Set the maximum amount of heap memory to use for sorting samples.
     * @param val Maximum size of heap in bytes.
     * @see SampleSorter::setHeapMax().
     */
    void setHeapMax(size_t val) { _heapMax = val; }

    size_t getHeapMax() const { return _heapMax; }

    /**
     * @param val If true, and heapSize exceeds heapMax,
     *   then wait for heapSize to be less then heapMax,
     *   which will block any SampleSources that are inserting
     *   samples into this sorter.  If false, then discard any
     *   samples that are received while heapSize exceeds heapMax.
     * @see SampleSorter::setHeapBlock().
     */
    void setHeapBlock(bool val) { _heapBlock = val; }

    bool getHeapBlock() const { return _heapBlock; }

protected:

    std::string _name;

    std::map<unsigned int, DSMSensor*> _sensorMap;

    std::map<SampleClient*, std::list<DSMSensor*> > _sensorsByClient;

    nidas::util::Mutex _sensorMapMutex;

    SampleSorter _inputSorter;

    SampleSorter _procSampSorter;

    size_t _unrecognizedSamples;

    std::list<const SampleTag*> _sampleTags;

    std::list<const DSMConfig*> _dsmConfigs;

    int _sorterLengthMsecs;

    size_t _heapMax;

    bool _heapBlock;

};

/*
 * Wrapper around a SampleSource, which provides the SampleInput interface.
 */
class SampleInputWrapper: public SampleInput
{
public:

    SampleInputWrapper(SampleSource* src): _src(src)
    {
    }

    std::string getName() const;

    // nidas::util::Inet4Address getRemoteInet4Address() const;
    const DSMConfig* getDSMConfig() const { return 0; }

    const std::list<const SampleTag*>& getSampleTags() const;

    void addSampleClient(SampleClient* clnt) throw();

    void removeSampleClient(SampleClient* clnt) throw();
    /**
     * Client wants samples from the process() method of the
     * given DSMSensor.
     */
    void addProcessedSampleClient(SampleClient* clnt,DSMSensor* snsr);

    void removeProcessedSampleClient(SampleClient* clnt, DSMSensor* snsr = 0);

    void setSorterLengthMsecs(int val) {}

    int getSorterLengthMsecs() const { return 0; }

    /**
     * Set the maximum amount of heap memory to use for sorting samples.
     * @param val Maximum size of heap in bytes.
     * @see SampleSorter::setHeapMax().
     */
    void setHeapMax(size_t val) {}

    size_t getHeapMax() const { return 0; }

    /**
     * @param val If true, and heapSize exceeds heapMax,
     *   then wait for heapSize to be less then heapMax,
     *   which will block any SampleSources that are inserting
     *   samples into this sorter.  If false, then discard any
     *   samples that are received while heapSize exceeds heapMax.
     * @see SampleSorter::setHeapBlock().
     */
    void setHeapBlock(bool val) {}

    bool getHeapBlock() const { return 0; }

private:

    SampleSource* _src;

};

/*
 * Wrapper around a SampleSource, which provides the SampleInput interface.
 */
class DSMSensorWrapper: public SampleInput
{
public:

    DSMSensorWrapper(DSMSensor* snsr): _snsr(snsr)
    {
    }

    std::string getName() const;

    const DSMConfig* getDSMConfig() const;

    const std::list<const SampleTag*>& getSampleTags() const;

    void addSampleClient(SampleClient* clnt) throw();

    void removeSampleClient(SampleClient* clnt) throw();
    /**
     * Client wants samples from the process() method of the
     * given DSMSensor.
     */
    void addProcessedSampleClient(SampleClient* clnt,DSMSensor* snsr);

    void removeProcessedSampleClient(SampleClient* clnt, DSMSensor* snsr = 0);

    void setSorterLengthMsecs(int val) {}

    int getSorterLengthMsecs() const { return 0; }

    /**
     * Set the maximum amount of heap memory to use for sorting samples.
     * @param val Maximum size of heap in bytes.
     * @see SampleSorter::setHeapMax().
     */
    void setHeapMax(size_t val) {}

    size_t getHeapMax() const { return 0; }

    /**
     * @param val If true, and heapSize exceeds heapMax,
     *   then wait for heapSize to be less then heapMax,
     *   which will block any SampleSources that are inserting
     *   samples into this sorter.  If false, then discard any
     *   samples that are received while heapSize exceeds heapMax.
     * @see SampleSorter::setHeapBlock().
     */
    void setHeapBlock(bool val) {}

    bool getHeapBlock() const { return 0; }

private:

    DSMSensor* _snsr;

};

/**
 * Extension of the interface to a SampleInput providing the
 * methods needed to establish the connection to the source
 * of samples (socket or files) and actually read samples
 * from the connection.
 */
class SampleInputReader: public SampleInput, public IOChannelRequester
{
public:

    virtual ~SampleInputReader() {}

    virtual void requestConnection(DSMService*)
        throw(nidas::util::IOException) = 0;

    virtual void init() throw(nidas::util::IOException) = 0;

    /**
     * Read a buffer of data, serialize the data into samples,
     * and distribute() samples to the receive() method of my SampleClients.
     * This will perform only one physical read of the underlying device
     * and so is appropriate to use when a select() has determined
     * that there is data availabe on our file descriptor.
     */
    virtual void readSamples() throw(nidas::util::IOException) = 0;

    /**
     * Blocking read of the next sample from the buffer. The caller must
     * call freeReference on the sample when they're done with it.
     */
    virtual Sample* readSample() throw(nidas::util::IOException) = 0;

    virtual void close() throw(nidas::util::IOException) = 0;

};

}}	// namespace nidas namespace core

#endif
