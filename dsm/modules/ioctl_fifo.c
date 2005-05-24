/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef __RTCORE_KERNEL__
#define __RTCORE_KERNEL__
#endif

#define __RTCORE_POLLUTED_APP__
#include <gpos_bridge/sys/gpos.h>
#include <rtl.h>
#include <rtl_unistd.h>
#include <rtl_fcntl.h>
#include <rtl_signal.h>
#include <rtl_errno.h>

#include <linux/slab.h>
#include <linux/list.h>

#include <ioctl_fifo.h>

RTLINUX_MODULE(ioctl_fifo);

/* #define DEBUG */

LIST_HEAD(ioctlList);

rtl_pthread_mutex_t listmutex = RTL_PTHREAD_MUTEX_INITIALIZER;

static unsigned char* inputbuf = 0;
static rtl_size_t inputbufsize = 0;

static void ioctlHandler(int sig, rtl_siginfo_t *siginfo, void *v);

static unsigned char ETX = '\003';
  
/**
 * Make a FIFO name. This must match what is done with a device prefix
 * on the user side.
 */
char* makeDevName(const char* prefix, const char* suffix,
	int num)
{
    char numstr[16];
    sprintf(numstr,"%d",num);
    char* name = rtl_gpos_malloc( strlen(prefix) + strlen(suffix) + strlen(numstr) + 6 );
    strcpy(name,"/dev/");
    strcat(name,prefix);
    strcat(name,suffix);
    strcat(name,numstr);
    return name;
}

/**********************************************************************
 * Open the FIFO.  Must be called from init_module, since we do
 * a rtl_gpos_malloc.
 **********************************************************************/
struct ioctlHandle* openIoctlFIFO(const char* devicePrefix,
	int boardNum,ioctlCallback_t* callback,
	int nioctls,struct ioctlCmd* ioctls)
{
    int l;
    struct ioctlHandle* handle = 
	(struct ioctlHandle*) rtl_gpos_malloc( sizeof(struct ioctlHandle) );

    handle->ioctlCallback = callback;
    handle->boardNum = boardNum;
    handle->nioctls = nioctls;
    handle->ioctls = ioctls;
    handle->inFifoName = 0;
    handle->inFifofd = -1;
    handle->outFifoName = 0;
    handle->outFifofd = -1;
    handle->bufsize = 0;
    handle->buf = 0;

    /* in and out are from the user perspective */
    handle->inFifoName = makeDevName(devicePrefix,"_ictl_",boardNum);
    rtl_printf("creating %s\n",handle->inFifoName);

    // remove broken device file before making a new one
    if (rtl_unlink(handle->inFifoName) < 0 && rtl_errno != RTL_ENOENT)
    	goto error;

#define OK_TO_EXIST
#ifdef OK_TO_EXIST
    if (rtl_mkfifo(handle->inFifoName, 0666) < 0 && rtl_errno != RTL_EEXIST) goto error;
#else
    if (rtl_mkfifo(handle->inFifoName, 0666)) goto error;
#endif

    if ((handle->inFifofd = rtl_open(handle->inFifoName, RTL_O_NONBLOCK | RTL_O_WRONLY)) < 0)
      goto error;

    handle->outFifoName = makeDevName(devicePrefix,"_octl_",boardNum);
    rtl_printf("creating %s\n",handle->outFifoName);

    // remove broken device file before making a new one
    if (rtl_unlink(handle->outFifoName) < 0)
      if (rtl_errno != RTL_ENOENT) goto error;

    if (rtl_mkfifo(handle->outFifoName, 0666) < 0) goto error;
    if ((handle->outFifofd = rtl_open(handle->outFifoName, RTL_O_NONBLOCK | RTL_O_RDONLY)) < 0)
      goto error;

    int icmd;
    for (icmd = 0; icmd < nioctls; icmd++)
    if (ioctls[icmd].size > handle->bufsize)
	handle->bufsize = ioctls[icmd].size;
    
    if (!(handle->buf = rtl_gpos_malloc( handle->bufsize ))) goto error;

    /* increase size of input buffer if necessary */
    l = handle->bufsize + sizeof(struct ioctlHeader) + 1;
    if (inputbufsize < l) {
	rtl_gpos_free(inputbuf);
	inputbufsize = l;
	if (!(inputbuf = rtl_gpos_malloc( inputbufsize ))) goto error;
    }
    	
    handle->bytesRead = handle->bytesToRead = 0;
    handle->icmd = -1;
    handle->readETX = 0;

    rtl_pthread_mutex_init(&handle->mutex,0);

    /* add this to the list before registering the signal handler.  */
    rtl_pthread_mutex_lock(&listmutex);
    list_add(&handle->list,&ioctlList);
    rtl_pthread_mutex_unlock(&listmutex);

    struct rtl_sigaction sigact;
    rtl_memset(&sigact,0,sizeof(sigact));
    sigact.sa_sigaction = ioctlHandler;
    sigact.sa_fd        = handle->outFifofd;
    rtl_sigemptyset(&sigact.sa_mask);
    sigact.sa_flags     = RTL_SA_RDONLY | RTL_SA_SIGINFO;
    if ( rtl_sigaction(RTL_SIGPOLL, &sigact, NULL ) != 0 ) {
	rtl_printf("Error in rtl_sigaction(RTL_SIGPOLL,...) for %s\n",
	    handle->outFifoName);
	goto error;
    }

    rtl_printf("openIoctlFIFO finished, handle=0x%x\n",(unsigned int)handle);

    return handle;
error:
    closeIoctlFIFO(handle);
    return 0;
}

/**********************************************************************
 * Close the FIFO. Done on module cleanup.
 **********************************************************************/
void closeIoctlFIFO(struct ioctlHandle* handle)
{
  rtl_pthread_mutex_lock(&listmutex);
  rtl_pthread_mutex_lock(&handle->mutex);

  if (handle->inFifofd >= 0) rtl_close(handle->inFifofd);
  if (handle->outFifofd >= 0) rtl_close(handle->outFifofd);

  if (handle->inFifoName) {
      rtl_unlink(handle->inFifoName);
      rtl_printf("removed %s\n", handle->inFifoName);
      rtl_gpos_free(handle->inFifoName);
  }
  if (handle->outFifoName) {
      rtl_unlink(handle->outFifoName);
      rtl_printf("removed %s\n", handle->outFifoName);
      rtl_gpos_free(handle->outFifoName);
  }

  rtl_gpos_free(handle->buf);

  list_del(&handle->list);

  rtl_pthread_mutex_unlock(&handle->mutex);

  rtl_gpos_free(handle);

  rtl_pthread_mutex_unlock(&listmutex);
}

/**
 * Send a error response back on the FIFO.
 * 4 byte non-zero rtl_errno int, 4 byte length, length message, ETX.
 */
static int sendError(int fifofd,int errval, const char* msg)
{
  rtl_printf("sendError, errval=%d,msg=%s\n",errval,msg);
  rtl_write(fifofd,&errval,sizeof(int));
  int len = strlen(msg) + 1;
  rtl_write(fifofd,&len,sizeof(int));
  rtl_write(fifofd,msg,len);
  rtl_write(fifofd,&ETX,1);
  return 0;
}

/**
 * Send a OK response back on the FIFO.
 * 4 byte int 0, 4 byte length, length byte buffer, ETX
 */
static int sendResponse(int fifofd,unsigned char* buf, int len)
{
  // rtl_printf("sendResponse, len=%d\n",len);
  int errval = 0;
  rtl_write(fifofd,&errval,sizeof(int));
  rtl_write(fifofd,&len,sizeof(int));
  if (len > 0) rtl_write(fifofd,buf,len);
  rtl_write(fifofd,&ETX,1);
  return 0;
}

/**
 * RTL_SIGPOLL handler for the ioctl FIFO, which is called when there
 * is data to be read on the FIFO from user space programs.
 * We look through our list of registered ioctlHandle structures for
 * one with the correct file descriptor, then read bytes from the FIFO.
 * Once we read the cmd value from the FIFO we then know the
 * direction of the ioctl request (get or set), and the length
 * of the data involved.
 */
static void ioctlHandler(int sig, rtl_siginfo_t *siginfo, void *v)
{
  int outFifofd = siginfo->si_fd;
  struct list_head *ptr;
  struct ioctlHandle* entry;
  struct ioctlHandle* handle = NULL;
  unsigned char* inbufptr;
  unsigned char* inbufeod;
  int icmd;
  int nread;
  int res;

#ifdef DEBUG
  rtl_printf("ioctlHandler entered, sig=%d, outFifofd=%d code=%d\n",
  	sig,outFifofd,siginfo->si_code);
#endif


  if (siginfo->si_code == RTL_POLL_OUT) {
#ifdef DEBUG
    rtl_printf("ignoring RTL_POLL_OUT\n");
#endif
    return;	// read at user end
  }

  rtl_pthread_mutex_lock(&listmutex);
  for (ptr = ioctlList.next; ptr != &ioctlList; ptr = ptr->next) {
    entry = list_entry(ptr,struct ioctlHandle, list);
    if (entry->outFifofd == outFifofd) {
      handle = entry;
      break;
    }
  }
  rtl_pthread_mutex_unlock(&listmutex);

#ifdef DEBUG
  rtl_printf("ioctlHandler looked for handle\n");
#endif

  if (handle == NULL) {
    rtl_printf("ioctlHandler can't find handle for FIFO file descriptor %d\n",
    	outFifofd);
    sendError(handle->inFifofd,RTL_EINVAL,"can't find handle for this FIFO");
    return;
  }

#ifdef DEBUG
  rtl_printf("ioctlHandler found handle = 0x%x\n",(unsigned int)handle);
#endif

  rtl_pthread_mutex_lock(&handle->mutex);

  if ((nread = rtl_read(outFifofd,inputbuf,inputbufsize)) < 0) {
    rtl_printf("ioctlHandler read failure on %s\n",handle->outFifoName);
    sendError(handle->inFifofd,rtl_errno,"error reading from FIFO");
    goto unlock;
  }
#ifdef DEBUG
  rtl_printf("ioctlHandler read %d bytes\n",nread);
#endif
  if (nread == 0) goto unlock;

  inbufeod = inputbuf + nread;

  /*
   * The data coming from user space on the FIFO is formatted as
   * follows:
   *   4 byte int containing the command - one of the ioctl
   *       cmds supported by the driver.
   *   4 byte int containing the port index, 0-(N-1), for a board
   *       with N ports (i.e. a serial port card).
   *   4 byte int size of the request.
   *   If the request is a set from the user side
   *       (_IOC_DIR is an IOC_WRITE) a buffer of the specified
   *       size will follow
   *   4 byte ETX ('\003' character).
   *  
   *   The ETX is there in case things get screwed up. After 
   *   reading the above from the FIFO we scan for an ETX
   *   to make sure we're in sync.
   */

  for (inbufptr = inputbuf; inbufptr < inbufeod; ) {

#ifdef DEBUG
    rtl_printf("chars left in buffer=%d\n",inbufeod-inbufptr);
#endif

    if (handle->readETX) {
#ifdef DEBUG
	rtl_printf("search for ETX\n");
#endif
      for (; inbufptr < inbufeod; )  {
#ifdef DEBUG
	rtl_printf("ETX search char=%d, ETX=%d\n",(int) *inbufptr,(int)ETX);
#endif
        if (*inbufptr++ == ETX) {
	    handle->readETX = 0;
#ifdef DEBUG
	    rtl_printf("read ETX\n");
#endif
	    break;
	}
	else rtl_printf("didn't find ETX\n");
      }
      if (inbufptr == inbufeod) break;
    }

#ifdef DEBUG
    rtl_printf("ioctlHandler bytesRead= %d\n",handle->bytesRead);
#endif

    for ( ; handle->bytesRead < sizeof(struct ioctlHeader) &&
	  inbufptr < inbufeod; )
      ((unsigned char*)&(handle->header))[handle->bytesRead++] = *inbufptr++;

#ifdef DEBUG
    rtl_printf("ioctlHandler bytesRead2= %d\n",handle->bytesRead);
#endif

    if (handle->bytesRead < sizeof(struct ioctlHeader))  break;

    /* We now have read the cmd, port number and buffer size from the FIFO */

#ifdef DEBUG
    rtl_printf("ioctlHandler cmd= %d\n",handle->header.cmd);
#endif

    if ((icmd = handle->icmd) < 0) {
      /* find out which cmd it is. */
      int cmd = handle->header.cmd;
#ifdef DEBUG
      rtl_printf("ioctlHandler cmd= %d\n",cmd);
#endif

      for (icmd = 0; icmd < handle->nioctls; icmd++)
	if (handle->ioctls[icmd].cmd == cmd) break;

#ifdef DEBUG
      rtl_printf("ioctlHandler icmd=%d, cmd= %d \n",icmd,cmd);
#endif
     if (icmd == handle->nioctls) {
	rtl_printf("ioctlHandler cmd= %d not supported\n",cmd);
	sendError(handle->inFifofd,RTL_EINVAL,"cmd not supported");
	handle->bytesRead = 0;
	handle->readETX = 1;
	handle->icmd = -1;
	continue;
      }
      handle->icmd = icmd;

      if (handle->header.size > handle->bufsize) {
	  sendError(handle->inFifofd,RTL_EINVAL,"size too large");
	  rtl_printf("header size, %d, larger than bufsize %d\n",
	  	handle->header.size,handle->bufsize);
	  handle->bytesRead = 0;
	  handle->readETX = 1;
	  handle->icmd = -1;
	  continue;
      }
    }

    /* user set: read data from fifo */
    if (_IOC_DIR(handle->header.cmd) & _IOC_WRITE) {
#ifdef DEBUG
      rtl_printf("ioctlHandler __IOC_WRITE\n");
#endif
      handle->bytesToRead = sizeof(struct ioctlHeader) +
      	handle->header.size;
#ifdef DEBUG
      rtl_printf("ioctlHandler _IOC_WRITE, bytesToRead=%d\n",
      	handle->bytesToRead);
#endif
      for (; handle->bytesRead < handle->bytesToRead && inbufptr < inbufeod; )
	handle->buf[handle->bytesRead++ - sizeof(struct ioctlHeader)] = *inbufptr++;

      if (handle->bytesRead < handle->bytesToRead) break;	/* not done */
    }

    /* call ioctl function on device */
#ifdef DEBUG
    rtl_printf("ioctlHandler calling ioctlCallback, port=%d\n",
        handle->header.port);
#endif
    res = handle->ioctlCallback(handle->header.cmd,
	  handle->boardNum,handle->header.port,handle->buf,
	  	handle->header.size);

    if (res < 0) sendError(handle->inFifofd,-res,"ioctl error");
    else if (_IOC_DIR(handle->header.cmd) & _IOC_READ) {
#ifdef DEBUG
	rtl_printf("ioctlHandler __IOC_READ\n");
#endif
	sendResponse(handle->inFifofd,handle->buf,res);
    }
    else sendResponse(handle->inFifofd,handle->buf,0);

    handle->bytesRead = 0;
    handle->readETX = 1;
    handle->icmd = -1;
  }
unlock:
  rtl_pthread_mutex_unlock(&handle->mutex);
#ifdef DEBUG
  rtl_printf("ioctlHandler returning\n");
#endif
  return;
}

void cleanup_module (void)
{
    rtl_gpos_free(inputbuf);
}
