/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_DYNLD_RAWSAMPLEOUTPUTSTREAM_H
#define NIDAS_DYNLD_RAWSAMPLEOUTPUTSTREAM_H

#include <nidas/dynld/SampleOutputStream.h>

namespace nidas { namespace dynld {

class RawSampleOutputStream: public SampleOutputStream
{
public:

    RawSampleOutputStream();

    RawSampleOutputStream(IOChannel* iochan);

    virtual ~RawSampleOutputStream();

    bool isRaw() const { return true; }

    void fromDOMElement(const xercesc::DOMElement* node)
	throw(nidas::util::InvalidParameterException);
protected:

    RawSampleOutputStream* clone(IOChannel* iochannel);

    RawSampleOutputStream(RawSampleOutputStream&,IOChannel*);

private:

    RawSampleOutputStream(const RawSampleOutputStream&);

};

}}	// namespace nidas namespace core

#endif
