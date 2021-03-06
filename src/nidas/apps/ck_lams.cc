/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*- */
/* vim: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 ********************************************************************
 ** NIDAS: NCAR In-situ Data Acquistion Software
 **
 ** 2006, Copyright University Corporation for Atmospheric Research
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** The LICENSE.txt file accompanying this software contains
 ** a copy of the GNU General Public License. If it is not found,
 ** write to the Free Software Foundation, Inc.,
 ** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **
 ********************************************************************
*/
/* ck_lams.cc

   Test program to print out messages received from the 
   FIFO buffers for the LAMS DSP card.

   Original Author: Mike Spowart

*/

// Linux include files.
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/select.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <bits/posix1_lim.h> // SSIZE_MAX
#include <signal.h>          // sigaction
#include <nidas/core/UnixIODevice.h>
#include <nidas/core/DSMSensor.h>
#include <nidas/linux/lams/lamsx.h>

using namespace std;
using namespace nidas::core;
namespace n_u = nidas::util;

#define err(format, arg...) \
     printf("%s: %s: " format "\n",__FILE__, __FUNCTION__ , ## arg)

#if defined(__arm__)

#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

int running = 1;

class LamsSensor : public DSMSensor
{
public:
  IODevice* buildIODevice() throw(n_u::IOException)
  {
    return new UnixIODevice();
  }
  SampleScanner* buildSampleScanner()
    throw(n_u::InvalidParameterException)
  {
    return new DriverSampleScanner();
  }
  bool process(const Sample* s, list<const Sample*>& result) throw()
  {
      return false;
  }
};
/* -------------------------------------------------------------------- */
void sigAction(int sig, siginfo_t* siginfo, void* vptr)
{
  cerr << "received signal " << strsignal(sig) << "(" << sig << ")" <<
    " si_signo=" << (siginfo ? siginfo->si_signo : -1) <<
    " si_errno=" << (siginfo ? siginfo->si_errno : -1) <<
    " si_code=" << (siginfo ? siginfo->si_code : -1) << endl;
  
  switch(sig) {
  case SIGHUP:
  case SIGTERM:
  case SIGINT:
    running = 0;
    err("running: %d", running);
    break;
  default:
    break;
  }
}

unsigned int nAVG   = 80;
unsigned int nPEAK  = 1000;

/* -------------------------------------------------------------------- */

int usage(const char* argv0)
{
    cerr << "Usage: " << argv0 << " [-a ... ] [-p ... ]\n\
-a: avererage number of spectra (default is " << nAVG << ")\n\
-p: peak clearing frequency     (default is " << nPEAK << ")\n\
\n";
    
    return 1;
}

int parseRunstring(int argc, char** argv)
{
    extern char *optarg;       /* set by getopt()  */
    extern int optind;         /*  "  "     "      */
    int opt_char;              /* option character */

    while ((opt_char = getopt(argc, argv, "ca:p:s:")) != -1) {

	switch (opt_char) {
	case 'a':
            nAVG = atoi(optarg);
	    break;
	case 'p':
            nPEAK = atoi(optarg);
	    break;
	case 'h':
	case '?':
	    return usage(argv[0]);
	}
    }
    return 0;
}

/* -------------------------------------------------------------------- */
int main(int argc, char** argv)
{
  err("ARM version - compiled on %s at %s", __DATE__, __TIME__);
  err("sizeof(lamsPort): %d\n", sizeof(lamsPort));

  int res = parseRunstring(argc,argv);
  if (res != 0) return res;

  string ofName("/mnt/lams/lams.bin");
  int ofPtr;
  err("nAVG: %d   nPEAK: %d", nAVG, nPEAK);

  // set up a sigaction to respond to ctrl-C
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset,SIGHUP);
  sigaddset(&sigset,SIGTERM);
  sigaddset(&sigset,SIGINT);
  sigprocmask(SIG_UNBLOCK,&sigset,(sigset_t*)0);

  struct sigaction act;
  sigemptyset(&sigset);
  act.sa_mask = sigset;
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = sigAction;
  sigaction(SIGHUP,&act,(struct sigaction *)0);
  sigaction(SIGINT,&act,(struct sigaction *)0);
  sigaction(SIGTERM,&act,(struct sigaction *)0);

  // create the board sensor
  LamsSensor sensor_in_0;
  sensor_in_0.setDeviceName("/dev/lams0");

  // open up the lams sensor
  err("opening: /dev/lams0");
  try {
    sensor_in_0.open(O_RDONLY);
    err("sensor_in_0.open(O_RDONLY) success!");
  }
  catch (n_u::IOException& ioe) {
    err("%s",ioe.what());
    goto failed;
  }
  int fd_lams_data;
  fd_lams_data = sensor_in_0.getReadFd();

  sensor_in_0.ioctl(LAMS_TAS_BELOW, 0, 0);
  sensor_in_0.ioctl(LAMS_N_AVG,     &nAVG,      sizeof(nAVG));
  sensor_in_0.ioctl(LAMS_N_PEAKS,   &nPEAK,     sizeof(nPEAK));

  // Set the lams channel (this starts the data xmit from the driver)
  struct lams_set set_lams;
  set_lams.channel = 1;
  sensor_in_0.ioctl(LAMS_SET_CHN, &set_lams, sizeof(set_lams));

  // Note: fd_set is a 1024 bit mask.
  fd_set readfds;

  // Main loop for gathering data from the lams channels
  int len, rlen, status, nsel;
  len = 0;
//unsigned int databuf[sizeof(set_lams)*2];
  char databuf[sizeof(lamsPort)];
  struct lamsPort* data; data = (lamsPort*) &databuf;

  int nfds;
  err("entering main loop...");
  int skip; skip = 0;
  int seq;  seq  = 0;
  while (running) {
    nfds = 0;

    // zero the readfds bitmask
    FD_ZERO(&readfds);

    // set the fd's to read data from ALL ports
    FD_SET(fd_lams_data, &readfds);
    nfds = max (nfds, fd_lams_data);

    // The select command waits for inbound FIFO data for ALL ports
    nsel = select(nfds+1, &readfds, NULL, NULL, 0);
    if (nsel < 0) // select error
      err("select error");

    // check to see if there is data on this FIFO
    if (FD_ISSET(fd_lams_data, &readfds)) {
      errno = 0;
      rlen = read(fd_lams_data, &databuf[len], sizeof(lamsPort));
      if (rlen < 0) {
        err("failed to read (%s), fd_lams_data: %x", strerror(errno), fd_lams_data);
        goto failed;
      }
      len += rlen;
      if (++skip == 8) {
        skip=0;
        err("rlen: %d data->avrg[%3d]: %x", rlen, seq, data->avrg[seq]);
        err("rlen: %d data->peak[%3d]: %x", rlen, seq, data->peak[seq]);
        if (++seq == LAMS_SPECTRA_SIZE) seq = 0;
      }
      if (len == sizeof(lamsPort)) {
        len = 0;
        errno = 0;

        // Open up the disk for writing lams data
//      err("creating: %s", ofName.c_str());
        ofPtr = creat(ofName.c_str(), 0666);
        if (ofPtr < 0) {
          err("failed to open '%s' (%s)", ofName.c_str(), strerror(errno));
          goto failed;
        }
        status = pwrite(ofPtr, data->avrg, sizeof(data->avrg), 0); 
        status = pwrite(ofPtr, data->peak, sizeof(data->peak), sizeof(data->avrg));
        close(ofPtr);
        if (status < 0) {
          err("failed to write (%s)", strerror(errno));
          goto failed;
        }
      }
    }
  }
failed:
  err("closing sensors...");

  if (ofPtr != -1) close(ofPtr);

  set_lams.channel = 0;
  sensor_in_0.ioctl(LAMS_SET_CHN, &set_lams, sizeof(set_lams));
  sensor_in_0.close();
  err("sensors closed.");
  return 1;
}

#else

int main(int argc, char** argv)
{
  char readbuf[sizeof(lamsPort)];
  char linebuf[11+LAMS_SPECTRA_SIZE*6];
  unsigned int n, nRead, nHead;

  struct lamsPort* data = (lamsPort*) &readbuf;

  err("X86 version - compiled on %s at %s", __DATE__, __TIME__);

  const char* home = getenv("HOME");

  string ifName, ofName;
  if (argc < 2) {
    ifName = string("/home/lams/lams.bin");
    ofName = string(home)+string("/lams/lams.dat");
  } else {
    ifName = string(argv[1]);
    ofName = string(argv[2]);
  }
  
  int ifPtr=-1, ofPtr=-1;
  ifPtr = open(ifName.c_str(), 0);
  if (ifPtr < 0) {
    err("failed to open '%s' (%s)", ifName.c_str(), strerror(errno));
    goto failed;
  }
  ofPtr = creat(ofName.c_str(), 0666);
  if (ofPtr < 0) {
    err("failed to create '%s' (%s)", ofName.c_str(), strerror(errno));
    goto failed;
  }
  do {
    nRead = read(ifPtr, &readbuf, sizeof(lamsPort));

    if ( nRead > 0 ) {

      nHead = 0;
//      sprintf(&linebuf[nHead], "%08lx", data->timetag); nHead+=8;
      for (n=0; n<LAMS_SPECTRA_SIZE; n++)
        sprintf(&linebuf[nHead+n*6], " %5d", data->avrg[n]);
      sprintf(&linebuf[nHead+n*6], "\n");

      int status = write(ofPtr, &linebuf, strlen(linebuf)); 
      if (status < 0) {
        err("failed to write (%s)", strerror(errno));
        goto failed;
      }
    }
  } while ( nRead > 0 );

failed:
  if (ofPtr != -1) close(ofPtr);
  if (ifPtr != -1) close(ifPtr);
  return 0;
}
#endif
