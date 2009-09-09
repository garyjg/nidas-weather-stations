/*
 ******************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$

 ******************************************************************
*/
#ifndef NIDAS_DYNLD_DSC_EVENT_H
#define NIDAS_DYNLD_DSC_EVENT_H

#include <nidas/core/DSMSensor.h>
#include <nidas/util/EndianConverter.h>

namespace nidas { namespace dynld {

using namespace nidas::core;

/**
 * Sensor support for a simple event detector.
 * This implementation supports a device that can
 * configured with a reporting period value ( e.g. 1 sec, 
 * or 1/10 sec).  Samples from the device are little-endian
 * 4 byte unsigned integer accumulated counts.
 * This class currently has hard-coded ioctl commands to
 * the gpio_mm device driver which supports a counter on a
 * Diamond GPIO card.
 */
class DSC_Event : public DSMSensor {

public:

    DSC_Event();
    ~DSC_Event();

    bool isRTLinux() const;

    IODevice* buildIODevice() throw(nidas::util::IOException);

    SampleScanner* buildSampleScanner();

    /**
     * Open the device connected to the sensor.
     */
    void open(int flags) throw(nidas::util::IOException,
        nidas::util::InvalidParameterException);

    void init() throw(nidas::util::InvalidParameterException);
                                                                                
    /*
     * Close the device connected to the sensor.
     */
    void close() throw(nidas::util::IOException);

    void printStatus(std::ostream& ostr) throw();

    /**
     * Process a raw sample, which in this case means 
     * convert the input counts to a float.
     */
    bool process(const Sample*,std::list<const Sample*>& result)
        throw();

private:

    dsm_sample_id_t _sampleId;

    const nidas::util::EndianConverter* _cvtr;

};

}}	// namespace nidas namespace dynld

#endif