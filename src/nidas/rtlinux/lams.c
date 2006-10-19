/*
 ********************************************************************
    Copyright by the National Center for Atmospheric Research

    $LastChangedDate: 2004-10-15 17:53:32 -0600 (Fri, 15 Oct 2004) $

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL: http://orion/svn/hiaper/ads3/dsm/class/RTL_DSMSensor.h $

    RTLinux LAMS driver for the ADS3 DSM.

 ********************************************************************
*/

#define __RTCORE_POLLUTED_APP__

#include <gpos_bridge/sys/gpos.h>
#include <rtl.h>
#include <rtl_pthread.h>
#include <rtl_semaphore.h>
#include <rtl_core.h>
#include <rtl_unistd.h>
#include <rtl_time.h>
#include <rtl_posixio.h>
#include <rtl_stdio.h>
#include <linux/ioport.h>

#include <nidas/rtlinux/dsmlog.h>
#include <nidas/rtlinux/dsm_viper.h>
#include <nidas/rtlinux/rtl_isa_irq.h>
#include <nidas/rtlinux/irigclock.h>
#include <nidas/rtlinux/ioctl_fifo.h>
#include <nidas/rtlinux/dsm_version.h>

#include <linux/kernel.h>
#include <linux/termios.h>
#include <nidas/rtlinux/lams.h>

RTLINUX_MODULE(lams);

MODULE_AUTHOR("John Wasinger <wasinger@ucar.edu>");
MODULE_DESCRIPTION("LAMS ISA driver for RTLinux");

// module parameters (can be passed in via command line)
static int irq = 4;
static int ioport = 0x220;

MODULE_PARM(irq, "i");
MODULE_PARM_DESC(irq, "ISA irq setting (default 4)");

MODULE_PARM(ioport, "i");
MODULE_PARM_DESC(ioport, "ISA memory base (default 0x220)");

volatile unsigned long baseAddr;
static const char* devprefix = "lams";
static struct ioctlHandle* ioctlHandle = 0;
static rtl_sem_t threadSem;
static rtl_pthread_t lamsThread = 0;
static int fd_lams_data = 0;
static struct lamsPort lams;

static struct ioctlCmd ioctlcmds[] = {
   { GET_NUM_PORTS,  _IOC_SIZE(GET_NUM_PORTS) },
   { AIR_SPEED,      _IOC_SIZE(AIR_SPEED)     },
   { LAMS_SET,       _IOC_SIZE(LAMS_SET)      },
};
static int nioctlcmds = sizeof(ioctlcmds) / sizeof(struct ioctlCmd);

// -- UTIL ---------------------------------------------------------------------
// TODO - share this with other modules
static char * createFifo(char inName[], int chan)
{
   char * devName;

   // create its output FIFO
   devName = makeDevName(devprefix, inName, chan);
   DSMLOG_DEBUG("rtl_mkfifo %s\n", devName);

   // remove broken device file before making a new one
   if ((rtl_unlink(devName) < 0 && rtl_errno != RTL_ENOENT)
       || rtl_mkfifo(devName, 0666) < 0)
   {
      DSMLOG_ERR("error: unlink/mkfifo %s: %s\n",
                 devName, rtl_strerror(rtl_errno));
      return 0;
   }
   return devName;
}
#if 0
void flush_it ()
{
   // flush the hardware data FIFO
   static int nGlyph;
   int n5=0, n4=0, n3=0, n2=0, n1=0, n0=0, nBad=0, nWhile=0;
   unsigned int data, flags;

   data  = readw(baseAddr + DATA_OFFSET);
   flags = readw(baseAddr + FLAGS_OFFSET);
      DSMLOG_DEBUG("data: 0x%04x flags: 0x%04x\n", data, flags);
   while (flags != FIFO_EMPTY) {
      data  = readw(baseAddr + DATA_OFFSET);
      flags = readw(baseAddr + FLAGS_OFFSET);
//    rtl_usleep(10);
      nWhile++;
      switch (flags) {
      case 5: // FIFO_FULL:
         n5++;
         break;
      case 4: // FIFO_FULL:
         n4++;
         break;
      case 3: // FIFO_EMPTY:
         n3++;
         break;
      case 2: // FIFO_EMPTY:
         n2++;
         break;
      case 1: // FIFO_EMPTY:
         n1++;
         break;
      case 0: // FIFO_EMPTY:
         n0++;
         break;
      default:
         nBad++;
         break;
      }
   }
      if (++nGlyph == 4) nGlyph = 0;
      DSMLOG_DEBUG("(%d) n5:%d n4:%d n3:%d n2:%d n1: %d n0: %d nBad: %d nWhile: %d\n",
                   nGlyph, n5, n4, n3, n2, n1, n0, nBad, nWhile);
}
#endif // 0
// -- THREAD -------------------------------------------------------------------
static void *lams_thread (void * chan)
{
   struct rtl_timespec timeout; // semaphore timeout in nanoseconds

   rtl_clock_gettime(RTL_CLOCK_REALTIME,&timeout);
   timeout.tv_sec = 0;
   timeout.tv_nsec = 0;

   for (;;) {
      timeout.tv_nsec += 300 * NSECS_PER_MSEC;
      if (timeout.tv_nsec >= NSECS_PER_SEC) {
         timeout.tv_sec++;
         timeout.tv_nsec -= NSECS_PER_SEC;
      }
      if (rtl_sem_timedwait(&threadSem, &timeout) < 0) {
         DSMLOG_DEBUG("thread timed out!\n");
         // timed out!  flush the hardware FIFO
         while ( readw(baseAddr + FLAGS_OFFSET) != FIFO_EMPTY )
            readw(baseAddr + DATA_OFFSET);
      } else {
         lams.timetag = GET_MSEC_CLOCK;
         if (fd_lams_data)
            rtl_write(fd_lams_data, &lams, sizeof(lams));
      }
      if (rtl_errno == RTL_EINTR) return 0; // thread interrupted
   }
}
/*
   unsigned int flags = 0, data = 0;
   static int glyph = 0;

   // this always shows flags: 0x0006
   for (;;) {
      rtl_usleep(1000000);
      if (rtl_errno == RTL_EINTR) break; // thread interrupted

      flush_it();
//    flags = readw(baseAddr + FLAGS_OFFSET);
//    data  = readw(baseAddr + DATA_OFFSET);
//    DSMLOG_DEBUG("(%d) flags: 0x%04x   data: 0x%04x\n", glyph, flags, data);
//    if (++glyph == 4) glyph=0;
   }
   // very tight loop...
   for (;;) {
      rtl_usleep(10000);
      if (rtl_errno == RTL_EINTR) break; // thread interrupted

      if (0x5 != (flags = readw(baseAddr + FLAGS_OFFSET))) {
         DSMLOG_DEBUG("(%d) bad flag: 0x%04x\n", glyph, flags);
         if (++glyph == 4) glyph=0;
      }
   }
*/
// -- INTERRUPT SERVICE ROUTINE ------------------------------------------------
static unsigned int lams_isr (unsigned int irq, void* callbackPtr,
                              struct rtl_frame *regs)
{
   static unsigned long sum[MAX_BUFFER];
   static int nAvg;
// static int nTattle, nGlyph;
   int n, flags;

   for (n=0; n<MAX_BUFFER; n++)
      sum[n] += readw(baseAddr + DATA_OFFSET);
   for (n=0; n<MAX_BUFFER; n++)
      readw(baseAddr + DATA_OFFSET);

   if (++nAvg == N_AVG) {
      for (n=0; n<MAX_BUFFER; n++) {
         lams.data[n] = sum[n] / N_AVG;
         sum[n] = 0;
      }
      nAvg = 0;
      rtl_sem_post( &threadSem );
   }
   n = 0;
   while ( (flags = readw(baseAddr + FLAGS_OFFSET)) != FIFO_EMPTY ) {
      readw(baseAddr + DATA_OFFSET);
//    n++;
   }
// if (++nTattle == 1024) {
//    nTattle = 0;
//    if (++nGlyph == 4) nGlyph = 0;
//    DSMLOG_DEBUG("(%d) nExtra: %d flags: %d sum: 0x%04x lams.data: 0x%04x\n",
//                 nGlyph, n, flags, sum[0], lams.data[0]);
// }
   return 0;
}
// -- IOCTL --------------------------------------------------------------------
static int ioctlCallback(int cmd, int board, int chn,
                         void *buf, rtl_size_t len)
{
   int ret = len;
   unsigned int airspeed, lams_channels;
   struct lams_set* lams_ptr;
   char devstr[30];

   switch (cmd)
   {
      case GET_NUM_PORTS:
         DSMLOG_DEBUG("GET_NUM_PORTS\n");
         *(int *) buf = N_CHANNELS;
         break;

      case AIR_SPEED:
         DSMLOG_DEBUG("AIR_SPEED\n");
         airspeed = *(unsigned int*) buf;
//       writew(airspeed, baseAddr + AIR_SPEED_OFFSET);
         break;

      case LAMS_SET:
         DSMLOG_DEBUG("LAMS_SET\n");
         lams_ptr = (struct lams_set*) buf;
         lams_channels = lams_ptr->channel;
         DSMLOG_DEBUG("LAMS_SET lams_channels=%d\n", lams_channels);

         // open the channel's data FIFO
         sprintf( devstr, "%s/lams_in_%d", getDevDir(), 0);
         if ((fd_lams_data =
            rtl_open( devstr, RTL_O_NONBLOCK | RTL_O_WRONLY )) < 0) {
               DSMLOG_ERR("error: open %s: %s\n", devstr,
                          rtl_strerror(rtl_errno));
            return -convert_rtl_errno(rtl_errno);
         }
         break;

      default:
         ret = -RTL_EIO;
         break;
   }
   return ret;
}
// -- CLEANUP MODULE -----------------------------------------------------------
void cleanup_module (void)
{
   DSMLOG_DEBUG("start\n");

   // close the RTL data fifo
   DSMLOG_DEBUG("closing fd_lams_data: %x\n", fd_lams_data);
   if (fd_lams_data)
      rtl_close( fd_lams_data );

   // destroy the RTL data fifo
   char devstr[30];
   sprintf( devstr, "%s/lams_in_%d", getDevDir(), 0);
   rtl_unlink( devstr );

   // undo what was done in reverse order upon cleanup
   rtl_free_isa_irq(irq);
   rtl_pthread_kill(lamsThread, SIGTERM);
   rtl_pthread_join(lamsThread, NULL);
   rtl_sem_destroy(&threadSem);
   release_region(baseAddr, REGION_SIZE);
   closeIoctlFIFO(ioctlHandle);

   DSMLOG_DEBUG("done\n");
}
// -- INIT MODULE --------------------------------------------------------------
int init_module (void)
{
   baseAddr = SYSTEM_ISA_IOPORT_BASE + ioport;
   DSMLOG_NOTICE("compiled on %s at %s\n", __DATE__, __TIME__);

   // open up ioctl FIFO, register ioctl function
   createFifo("_in_", BOARD_NUM);
   ioctlHandle = openIoctlFIFO(devprefix, BOARD_NUM, ioctlCallback,
                               nioctlcmds, ioctlcmds);
   if (!ioctlHandle) return -RTL_EIO;

   request_region(baseAddr, REGION_SIZE, "lams");

   // initialize the semaphore to the thread
   if ( rtl_sem_init( &threadSem, 1, 0) < 0) {
      DSMLOG_ERR("rtl_sem_init failure: %s\n", rtl_strerror(rtl_errno));
      goto rtl_sem_init_err;
   }
   // create the thread to write data to the FIFO
   if (rtl_pthread_create( &lamsThread, NULL,
                           lams_thread, (void *)0)) {
      DSMLOG_ERR("rtl_pthread_create failure: %s\n", rtl_strerror(rtl_errno));
      goto rtl_pthread_create_err;
   }
   // activate interupt service routine
   while ( readw(baseAddr + FLAGS_OFFSET) != FIFO_EMPTY ) // pre-enable-flush
      readw(baseAddr + DATA_OFFSET);
   if (rtl_request_isa_irq(irq, lams_isr, 0) < 0) {
      DSMLOG_ERR("rtl_request_isa_irq failure: %s\n", rtl_strerror(rtl_errno));
      goto rtl_request_isa_irq_err;
   }
   while ( readw(baseAddr + FLAGS_OFFSET) != FIFO_EMPTY ) // post-enable-flush
      readw(baseAddr + DATA_OFFSET);

   DSMLOG_DEBUG("done\n");
   return 0;

   // undo what was done in reverse order upon failure
   rtl_request_isa_irq_err: rtl_free_isa_irq(irq);
                            rtl_pthread_kill(lamsThread, SIGTERM);
                            rtl_pthread_join(lamsThread, NULL);
   rtl_pthread_create_err:  rtl_sem_destroy(&threadSem);
   rtl_sem_init_err:        release_region(baseAddr, REGION_SIZE);
                            closeIoctlFIFO(ioctlHandle);
   return -convert_rtl_errno(rtl_errno);
}
