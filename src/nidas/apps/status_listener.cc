/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************
*/
#include <nidas/core/StatusListener.h>
#include <nidas/core/Datagrams.h>
#include <nidas/util/IOException.h>

#include <iostream>
#include <string>

using namespace nidas::core;
using namespace std;

namespace n_u = nidas::util;

int main(int argc, char** argv)
{
  // start up the socket listener thread
  cerr << "StatusListener lstn();" << endl;
  StatusListener lstn;
  try {
    lstn.start();
  } catch (n_u::Exception& e) {
    cerr << "n_u::Exception: " << e.toString() << endl;
    return 1;
  }
  // Create an XMLRPC web service
  XmlRpc::XmlRpcServer* xmlrpc_server = new XmlRpc::XmlRpcServer;

  // These constructors register methods with the XMLRPC server
  GetClocks getclocks(xmlrpc_server, &lstn);
  GetStatus getstatus(xmlrpc_server, &lstn);

  // DEBUG - set verbosity of the xmlrpc server HIGH...
//   XmlRpc::setVerbosity(5);

  // Create the server socket on the specified port
  if (!xmlrpc_server->bindAndListen(NIDAS_XMLRPC_STATUS_PORT_TCP)) {
        n_u::IOException e("XMLRPC status port","bind",errno);
        cerr << e.what() << endl;
        return 1;
  }

  // Enable introspection
  xmlrpc_server->enableIntrospection(true);

  // Wait for requests indefinitely
  xmlrpc_server->work(-1.0);

  return 1;
}
