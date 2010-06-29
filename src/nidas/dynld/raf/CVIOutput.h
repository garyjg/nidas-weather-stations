/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate: 2007-01-31 11:23:38 -0700 (Wed, 31 Jan 2007) $

    $LastChangedRevision: 3648 $

    $LastChangedBy: cjw $

    $HeadURL: http://svn/svn/nids/trunk/src/nidas/dynld/CVIOutput.h $
 ********************************************************************

*/

#ifndef NIDAS_DYNLD_RAF_CVIOUTPUT_H
#define NIDAS_DYNLD_RAF_CVIOUTPUT_H

#include <nidas/core/SampleOutput.h>
#include <nidas/core/DerivedDataClient.h>

#include <iostream>

namespace nidas {

namespace core {
class Variable;
}

namespace dynld { namespace raf {

using namespace nidas::core;

class CVIOutput: public SampleOutputBase, public DerivedDataClient
{
public:

    CVIOutput();

    CVIOutput(IOChannel* iochannel);

    ~CVIOutput();

    CVIOutput* clone(IOChannel* iochannel=0);

    void addRequestedSampleTag(SampleTag*)
        throw(nidas::util::InvalidParameterException);

    void requestConnection(SampleConnectionRequester* requester) throw();

    bool receive(const Sample* samp) throw();

    void derivedDataNotify(const DerivedDataReader * s) throw();

protected:
    /**
     * Copy constructor, with a new IOChannel.
     */
    CVIOutput(CVIOutput&,IOChannel*);

private:

    std::ostringstream ostr;

    std::vector<const Variable*> _variables;

    dsm_time_t _tt0;

    /**
     * True airspeed from DerivedDataReader.
     */
    float _tas;

};

}}}	// namespace nidas namespace dynld namespace raf

#endif
