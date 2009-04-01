/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/

#include <nidas/core/DSMServer.h>
#include <nidas/util/Logger.h>

using namespace nidas::core;

int main(int argc, char** argv)
{
    int res = DSMServer::main(argc,argv);
    ILOG(("dsm_server exiting, status=") << res);
    return res;
}
