/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/

#include <nidas/core/ConnectionRequester.h>
#include <nidas/core/SampleInputHeader.h>
#include <nidas/core/Version.h>
#include <nidas/core/Project.h>
#include <nidas/core/Site.h>

using namespace nidas::core;
using namespace std;

namespace n_u = nidas::util;

void SampleConnectionRequester::sendHeader(dsm_time_t thead,
	SampleOutput* output)
	throw(n_u::IOException)
{
    cerr << "ConnectionRequester::sendHeader" << endl;
    SampleInputHeader header;
    header.setArchiveVersion(Version::getArchiveVersion());
    header.setSoftwareVersion(Version::getSoftwareVersion());
    header.setProjectName(Project::getInstance()->getName());

    string sysname = Project::getInstance()->getSystemName();
    header.setSystemName(sysname);

    header.setConfigName(Project::getInstance()->getConfigName());
    header.setConfigVersion(Project::getInstance()->getConfigVersion());
    header.write(output);
}


