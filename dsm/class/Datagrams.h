/*
 ********************************************************************
    Copyright by the National Center for Atmospheric Research

    $LastChangedDate: 2004-10-15 17:53:32 -0600 (Fri, 15 Oct 2004) $

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL: http://orion/svn/hiaper/ads3/dsm/class/RTL_DSMSensor.h $
 ********************************************************************

*/

#ifndef DSM_DATAGRAMS_H
#define DSM_DATAGRAMS_H

#include <atdUtil/ServiceRequestDatagram.h>

#define DSM_MULTICAST_PORT 50000
#define DSM_MULTICAST_ADDR "239.0.0.10"

namespace dsm {

typedef enum datagramTypes {
    XML_CONFIG,
    RAW_SAMPLE,
    SYNC_RECORD,
} datagramType_t;


class XMLConfigRequestDatagram: public atdUtil::ServiceRequestDatagram
{
public:
    XMLConfigRequestDatagram():ServiceRequestDatagram(XML_CONFIG) {}
};

class RawSampleRequestDatagram: public atdUtil::ServiceRequestDatagram
{
public:
    RawSampleRequestDatagram(): ServiceRequestDatagram(RAW_SAMPLE) {}
};

}

#endif
