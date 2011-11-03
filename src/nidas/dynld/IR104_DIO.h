/*
 ******************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$

 ******************************************************************
*/
#ifndef NIDAS_DYNLD_IR104_DIO_H
#define NIDAS_DYNLD_IR104_DIO_H

#include <nidas/linux/diamond/ir104.h>

#include <nidas/core/DOMable.h>

#include <nidas/util/IOException.h>
#include <nidas/util/InvalidParameterException.h>
#include <nidas/util/BitArray.h>

namespace nidas { namespace dynld {

/**
 * Support for the digital IO on a Diamond Systems IR104 board.
 * There are 20 independent opto-isolated inputs, and 20 independent
 * relay outputs.  The value of the inputs can be read, and the value of
 * the outputs written or read.
 */
class IR104_DIO {

public:

    IR104_DIO();

    ~IR104_DIO();

    void setDeviceName(const std::string& val)
    {
        _devName = val;
    }

    const std::string& getDeviceName() const
    {
        return _devName;
    }

    const std::string& getName() const
    {
        return _devName;
    }

    const int getFd() const { return _fd; }

    /**
     * Open the DIO device.
     */
    void open() throw(nidas::util::IOException);

    /**
     * Close the DIO device.
     */
    void close() throw(nidas::util::IOException);

    /**
     * Return number of DOUT pins on this device (8).
     * Value will be 0 if the device has not been opened,
     * or -1 if the open failed.
     */
    int getNumOutputs() const 
    {
        return _noutputs;
    }

    /**
     * Return number of DIN pins on this device (8).
     * Value will be 0 if the device has not been opened,
     * or -1 if the open failed.
     */
    int getNumInputs() const 
    {
        return _ninputs;
    }

    /**
     * Clear, to low state, viper digital output ports OUT0-7,
     * as selected by bits 0-7 of which.
     */
    void clearOutputs(const nidas::util::BitArray& which) 
        throw(nidas::util::IOException,
                nidas::util::InvalidParameterException);

    /**
     * Set, to high state, viper digital output ports OUT0-7,
     * as selected by bits 0-7 of which
     */
    void setOutputs(const nidas::util::BitArray& which) 
        throw(nidas::util::IOException,
                nidas::util::InvalidParameterException);

    /**
     * Set ports OUT0-7, selected by bits 0-7 of which,
     * to 0(low) or high(1) based on bits 0-7 of val
     */
    void setOutputs(const nidas::util::BitArray& which,
        const nidas::util::BitArray& val) 
        throw(nidas::util::IOException,
                nidas::util::InvalidParameterException);

    /**
     * Get current settings of OUT0-7
     */ 
    nidas::util::BitArray getOutputs() throw(nidas::util::IOException);

    /**
     * get current settings of IN0-7
     */
    nidas::util::BitArray getInputs() throw(nidas::util::IOException);

private:

    std::string _devName;

    /**
     * The file descriptor of this device.
     */
    int _fd;

    /**
     * How many DOUT pins are on this device?
     */
    int _noutputs;

    /**
     * How many DIN pins are on this device?
     */
    int _ninputs;

};

}}	// namespace nidas namespace dynld

#endif
