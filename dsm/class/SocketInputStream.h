/*
 ********************************************************************
    Copyright by the National Center for Atmospheric Research

    $LastChangedDate: 2004-10-15 17:53:32 -0600 (Fri, 15 Oct 2004) $

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL: http://orion/svn/hiaper/ads3/dsm/class/RTL_DSMSensor.h $
 ********************************************************************

*/

#ifndef DSM_SOCKETINPUTSTREAM_H
#define DSM_SOCKETINPUTSTREAM_H

#include <InputStream.h>
#include <atdUtil/Socket.h>

#include <string>
#include <iostream>

namespace dsm {

/**
 * Implementation of an InputStream over an atdUtil::Socket.
 */
class SocketInputStream: public InputStream {

public:
  SocketInputStream(atdUtil::Socket& socket) :
  	InputStream(socket.getSendBufferSize()),sock(socket) {}

  /**
   * Do the actual hardware read.
   */
  size_t devRead(void* buf, size_t len) throw (atdUtil::IOException)
  {
      std::cerr << "SocketInputStream::devRead, len=" << len << std::endl;
      return sock.recv(buf,len);
  }

protected:
  atdUtil::Socket sock;
};

}

#endif
