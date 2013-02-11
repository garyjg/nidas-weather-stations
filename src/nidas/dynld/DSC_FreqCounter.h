// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*-
// vim: set shiftwidth=4 softtabstop=4 expandtab:
/*
 ******************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$

 ******************************************************************
*/
#ifndef NIDAS_DYNLD_DSC_FREQCOUNTER_H
#define NIDAS_DYNLD_DSC_FREQCOUNTER_H

#include <nidas/core/DSMSensor.h>
#include <nidas/core/Parameter.h>
#include <nidas/util/EndianConverter.h>

namespace nidas { namespace dynld {

using namespace nidas::core;

/**
 * Sensor support for a frequency counter device.
 * This implementation supports a device that can
 * configured with a reporting period value ( e.g. 1 sec, 
 * or 1/10 sec), and a number of pulses to count over that time.
 * Samples from the device are two little-endian
 * 4 byte unsigned integers, containing the number of pulses
 * counted along with a number of clock ticks elapsed while
 * counting those pulses.
 */
class DSC_FreqCounter : public DSMSensor {

public:

    DSC_FreqCounter();
    ~DSC_FreqCounter();

    IODevice* buildIODevice() throw(nidas::util::IOException);

    SampleScanner* buildSampleScanner()
        throw(nidas::util::InvalidParameterException);

    /**
     * Open a GPIO-MM frequency counter.
     */
    void open(int flags) throw(nidas::util::IOException,
        nidas::util::InvalidParameterException);

    void validate() throw(nidas::util::InvalidParameterException);

    void init() throw(nidas::util::InvalidParameterException);

    void printStatus(std::ostream& ostr) throw();

    /**
     * Calculate the input pulse period in microseconds.
     */
    double calculatePeriodUsec(const Sample*) const;

    /**
     * Calculate the input pulse period in microseconds,
     * based on the number of pulses counted (npulses) and the number
     * of clock tics counted while counting npulses.
     */
    double calculatePeriodUsec(unsigned int npulses, unsigned tics) const;

    /**
     * Process a raw sample, which in this case means convert the
     * counts and elapsed ticks into a frequency.
     */
    bool process(const Sample*,std::list<const Sample*>& result)
        throw();

    /**
     * Return the frequency sample period.
     */
    int getSamplePeriodMsec() const
    {
        return _msecPeriod;
    }


protected:

    virtual void readParams(const std::list<const Parameter*>& params)
        throw(nidas::util::InvalidParameterException);

    dsm_sample_id_t _sampleId;

    int _nvars;

    int _msecPeriod;

    /**
     * Number of input pulses to count.
     */
    int _numPulses;

    /**
     * Rate of reference clock whose tics are counted while
     * _numPulses are counted.
     */
    double _clockRate;

    const nidas::util::EndianConverter* _cvtr;

private:

    /** No copying. */
    DSC_FreqCounter(const DSC_FreqCounter&);

    /** No assignment. */
    DSC_FreqCounter& operator=(const DSC_FreqCounter&);

};

}}	// namespace nidas namespace dynld

#endif
