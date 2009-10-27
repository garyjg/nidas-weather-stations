/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/

#ifndef NIDAS_DYNLD_ISFF_GOESPROJECT_H
#define NIDAS_DYNLD_ISFF_GOESPROJECT_H

// #include <nidas/dynld/isff/Packets.h>
// #include <nidas/dynld/SampleInputStream.h>
#include <nidas/core/Project.h>

namespace nidas { namespace dynld { namespace isff {

using namespace nidas::dynld;	// put this within namespace block
using namespace nidas::core;	// put this within namespace block

class GOESProject
{
public:
    GOESProject(Project*p) throw(nidas::util::InvalidParameterException);
    ~GOESProject();

    Project* getProject() const { return _project; }

    /**
     * Get the station number, corresponding to a GOES id.
     * @return non-negative number.
     * Throws InvalidParameterException if there is
     * no "goes_ids" integer parameter for the Project,
     * or the goesId is not valid.
     */
    int getStationNumber(unsigned long goesId) const
	    throw(nidas::util::InvalidParameterException);

    int getXmitInterval(int stationNumber) const
	    throw(nidas::util::InvalidParameterException);

    int getXmitOffset(int stationNumber) const
	    throw(nidas::util::InvalidParameterException);

    const SampleTag* getGOESSampleTag(int stationNumber) const
	    throw(nidas::util::InvalidParameterException);

    void addSampleTag(SampleTag* tag) throw()
    {
        _sampleTags.push_back(tag);
        _constSampleTags.push_back(tag);
    }

    /**
     * Get a SampleTag*, corresponding to station and sampleid.
     * @return 0: SampleTag corresponding to a dsm id of stationNumber+1,
	    and the given sampleId not found.
     */
    const SampleTag* getSampleTag(int stationNumber, int sampleId) const;

    std::list<const SampleTag*> getSampleTags() const
    {
        return _constSampleTags;
    }

    unsigned long getGOESId(int stationNum) const
	throw(nidas::util::InvalidParameterException);

private:
    GOESProject(const GOESProject& x); 	// no copying
    
    GOESProject& operator=(const GOESProject& x) const; 	// no assign

    void readGOESIds()
	throw(nidas::util::InvalidParameterException);

    Project* _project;

    std::vector<unsigned long> _goesIds;

    std::map<unsigned long,int> _stationNumbersById;

    std::map<dsm_sample_id_t,const SampleTag*> _sampleTagsById;

    std::vector<int> _xmitOffsets;

    std::vector<int> _xmitIntervals;

    std::list<SampleTag*> _sampleTags;

    std::list<const SampleTag*> _constSampleTags;

    std::vector<SampleTag*> _goesTags;

};

}}}	// namespace nidas namespace dynld namespace isff

#endif
