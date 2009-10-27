/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$

    Linux driver module for Diamond Emerald serial IO cards.
    This just queries and sets the ioport addresses and irqs of the UARTs.
    The normal linux serial driver for the 8250 family of UARTs does the heavy work.

    This driver also supports the 8 digital I/O lines on a Diamond Emerald card.
    Each digio line is accessed via a minor number, ranging from 0 to 7 for the
    eight lines. Each digio line then has an associated device, which can then
    mirror the serial port names:
        serial port: /dev/ttyS9 
        digio line: /dev/ttyD9 

    Any of the minor device numbers 0-7 can be used when doing any of the ioctls to
    set/get parameters on the whole board.

 ********************************************************************
*/

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioport.h>
#include <linux/sched.h>    /* schedule() */
#include <asm/io.h>		/* outb, inb */
#include <asm/uaccess.h>	/* access_ok */

#include "emerald.h"	/* local definitions */

#include <nidas/linux/isa_bus.h>

static unsigned long ioport_base = SYSTEM_ISA_IOPORT_BASE;

static int emerald_major = EMERALD_MAJOR;
static unsigned int ioports[EMERALD_MAX_NR_DEVS] = {0,0,0,0};
static int emerald_nr_addrs = 0;
static int emerald_nr_ok = 0;
static emerald_board* emerald_boards = 0;
static emerald_port* emerald_ports = 0;
static int emerald_nports = 0;

module_param(emerald_major, int, S_IRUGO);
#if defined(module_param_array) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
module_param_array(ioports, int, &emerald_nr_addrs, S_IRUGO);	/* io port virtual address */
#else
module_param_array(ioports, int, emerald_nr_addrs, S_IRUGO);	/* io port virtual address */
#endif

MODULE_AUTHOR("Gordon Maclean");
MODULE_DESCRIPTION("driver module supporting initialization and digital I/O on Diamond System Emerald serial port card");
MODULE_LICENSE("Dual BSD/GPL");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#define mutex_init(x)               init_MUTEX(x)
#define mutex_lock_interruptible(x) ( down_interruptible(x) ? -ERESTARTSYS : 0)
#define mutex_unlock(x)             up(x)
#endif

/* 
 * To detect if compiling for VIPER
 * check ifdef __LINUX_ARM_ARCH
 * or CONFIG_ARCH_VIPER
 */

/*
 * return 0 if config is not OK.
 * According to the Emerald-MM-8 User Manual, v2.4,
 * valid port addresses are 0x100-0x3f8.
 * Valid irqs are 2,3,4,5,6,7,10,11,12 and 15.
 * On the viper, ISA irqs 2 & 15 are not mapped.
 */
static int emerald_check_config(emerald_config* config) {
        int i,j;
#ifdef CONFIG_ARCH_VIPER
        int valid_irqs[]={3,4,5,6,7,10,11,12};
        int nvalid = 8;
#else
        int valid_irqs[]={2,3,4,5,6,7,10,11,12,15};
        int nvalid = 10;
#endif

        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                // printk(KERN_INFO "emerald: ioport=%x\n",config->ports[i].ioport);
                if (config->ports[i].ioport > 0x3f8) return 0;
                if (config->ports[i].ioport < 0x100) return 0;
                for (j = 0; j < nvalid; j++) {
                        // printk(KERN_INFO "emerald: checking irq=%d against %d\n",
                          // config->ports[i].irq,valid_irqs[j]);
                        if (valid_irqs[j] == config->ports[i].irq) break;
                }
                if (j == nvalid) return 0;
        }
        return 1;
}

static void emerald_enable_ports(emerald_board* brd)
{
        outb(0x80,brd->ioport+EMERALD_APER);	/* enable ports */
}


static int emerald_read_config(emerald_board* brd)
{
        int i,val;
        /* read ioport values. */
        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                outb(i,brd->ioport+EMERALD_APER);
                val = inb(brd->ioport+EMERALD_ARR);
                if (val == 0xff) return -ENODEV;
                brd->config.ports[i].ioport = val << 3;

                // According to the Emerald manual the IRQ registers
                // cannot be read back. It does work just after
                // a emerald_write_config, but not after a bootup.
#ifdef TRY_READ_IRQ_FROM_REGISTER
                outb(i+EMERALD_NR_PORTS,brd->ioport+EMERALD_APER);
                val = inb(brd->ioport+EMERALD_ARR);
                if (val == 0xff) return -ENODEV;
                brd->config.ports[i].irq = val;
#endif
        }
        emerald_enable_ports(brd);
        return 0;
}

static int emerald_write_config(emerald_board* brd,emerald_config* config)
{
        int i;
        /* write ioport and irq values. */
        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                outb(i,brd->ioport+EMERALD_APER);
                outb(config->ports[i].ioport >> 3,brd->ioport+EMERALD_AIDR);

                outb(i+EMERALD_NR_PORTS,brd->ioport+EMERALD_APER);
                outb(config->ports[i].irq,brd->ioport+EMERALD_AIDR);
        }
        // copy config because we can't read IRQ values from registers
        brd->config = *config;
        emerald_enable_ports(brd);
        return 0;
}

/*
 * Read the ioport and irq configuration for the 8 serial ports
 * from the Emerald EEPROM.
 */
static int emerald_read_eeconfig(emerald_board* brd,emerald_config* config)
{
        int i;
        unsigned char busy;
        int ntry;

        /* get ioport values from EEPROM addresses 0-7 */
        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                outb(i,brd->ioport+EMERALD_ECAR);
                /* wait for busy bit in EMERALD_EBR to clear */
                ntry = 5;
                do {
                        unsigned long jwait = jiffies + 1;
                        while (time_before(jiffies,jwait)) schedule();
                        busy = inb(brd->ioport+EMERALD_EBR);
                        if (busy == 0xff) return -ENODEV;
                } while(busy & 0x80 && --ntry);
                if (!ntry) return -ETIMEDOUT;
                config->ports[i].ioport = (int)inb(brd->ioport+EMERALD_EDR) << 3;
        }

        /* get irq values from EEPROM addresses 8-15 */
        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                outb(i+EMERALD_NR_PORTS,brd->ioport+EMERALD_ECAR);
                /* wait for busy bit in EMERALD_EBR to clear */
                ntry = 5;
                do {
                        unsigned long jwait = jiffies + 1;
                        while (time_before(jiffies,jwait)) schedule();
                        busy = inb(brd->ioport+EMERALD_EBR);
                } while(busy & 0x80 && --ntry);
                if (!ntry) return -ETIMEDOUT;
                config->ports[i].irq = inb(brd->ioport+EMERALD_EDR);
        }
        return 0;
}

/*
 * Write the ioport and irq configuration for the 8 serial ports
 * to the Emerald EEPROM.
 */
static int emerald_write_eeconfig(emerald_board* brd,emerald_config* config)
{
        int i;
        unsigned char busy;
        int ntry;

        /* write ioport values to EEPROM addresses 0-7 */
        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                outb(config->ports[i].ioport >> 3,brd->ioport+EMERALD_EDR);
                outb(i + 0x80,brd->ioport+EMERALD_ECAR);

                /* wait for busy bit in EMERALD_EBR to clear */
                ntry = 5;
                do {
                        unsigned long jwait = jiffies + 1;
                        while (time_before(jiffies,jwait)) schedule();
                        busy = inb(brd->ioport+EMERALD_EBR);
                } while(busy & 0x80 && --ntry);
                if (!ntry) return -ETIMEDOUT;
        }

        /* write irq values to EEPROM addresses 8-15 */
        for (i = 0; i < EMERALD_NR_PORTS; i++) {
                outb(config->ports[i].irq,brd->ioport+EMERALD_EDR);
                outb(i + EMERALD_NR_PORTS + 0x80,brd->ioport+EMERALD_ECAR);

                /* wait for busy bit in EMERALD_EBR to clear */
                ntry = 5;
                do {
                        unsigned long jwait = jiffies + 1;
                        while (time_before(jiffies,jwait)) schedule();
                        busy = inb(brd->ioport+EMERALD_EBR);
                } while(busy & 0x80 && --ntry);
                if (!ntry) return -ETIMEDOUT;
        }
        return 0;
}

/*
 * Load the the ioport and irq configuration from the Emerald
 * EEPROM into registers.
 */
static int emerald_load_config_from_eeprom(emerald_board* brd)
{

        unsigned char busy;
        int ntry;
        int result = 0;

        outb(0x80,brd->ioport+EMERALD_CRR);	/* reload configuration from eeprom */

        /* wait for busy bit in EMERALD_EBR to clear */
        ntry = 5;
        do {
                unsigned long jwait = jiffies + 1;
                while (time_before(jiffies,jwait)) schedule();
                busy = inb(brd->ioport+EMERALD_EBR);
        } while(busy & 0x80 && --ntry);
        if (!ntry) return -ETIMEDOUT;

        // read back. Must get IRQs from EEPROM
        result = emerald_read_eeconfig(brd,&brd->config);
        // re-read registers to make sure
        if (!result) result = emerald_read_config(brd);
        return result;
}

static int emerald_get_digio_port_out(emerald_board* brd,int port)
{
        return (brd->digioout & (1 << port)) != 0;
}

static void emerald_set_digio_out(emerald_board* brd,int val)
{
        outb(val,brd->ioport+EMERALD_DDR);
        brd->digioout = val;
}

static void emerald_set_digio_port_out(emerald_board* brd,int port,int val)
{
        if (val) brd->digioout |= 1 << port;
        else brd->digioout &= ~(1 << port);
        outb(brd->digioout,brd->ioport+EMERALD_DDR);
}

static int emerald_read_digio(emerald_board* brd)
{
        brd->digioval = inb(brd->ioport+EMERALD_DIR);
        return brd->digioval;
}

static int emerald_read_digio_port(emerald_board* brd,int port)
{
        int val = emerald_read_digio(brd);
        return (val & (1 << port)) != 0;
}

static void emerald_write_digio_port(emerald_board* brd,int port,int val)
{
        if (val) brd->digioval |= 1 << port;
        else brd->digioval &= ~(1 << port);

        // this does not effect digital input lines
        outb(brd->digioval,brd->ioport+EMERALD_DOR);
}

#ifdef EMERALD_DEBUG /* use proc only if debugging */
/*
 * The proc filesystem: function to read
 */
                                                                                
static int emerald_read_procmem(char *buf, char **start, off_t offset,
                   int count, int *eof, void *data)
{
        int i, j, len = 0;
        int limit = count - 80; /* Don't print more than this */
        PDEBUGG("read_proc, count=%d\n",count);
                                                                                    
        for (i = 0; i < emerald_nr_ok && len <= limit; i++) {
                struct emerald_board *brd = emerald_boards + i;
                PDEBUGG("read_proc, i=%d, device=0x%lx\n",i,(unsigned long)brd);
                len += sprintf(buf+len,"\nDiamond Emerald-MM-8 %i: ioport %lx\n",
                               i, brd->ioport);
                /* loop over serial ports */
                for (j = 0; len <= limit && j < EMERALD_NR_PORTS; j++) {
                        len += sprintf(buf+len, "  port %d, ioport=%x,irq=%d\n",
                            j,brd->config.ports[j].ioport,brd->config.ports[j].irq);
                }
        }
        *eof = 1;
        return len;
}

static void emerald_create_proc(void)
{
        PDEBUGG("within emerald_create_proc\n");
        create_proc_read_entry("emerald", 0644 /* default mode */,
                               NULL /* parent dir */, emerald_read_procmem,
                               NULL /* client data */);
}
                                                                                
static void emerald_remove_proc(void)
{
        /* no problem if it was not registered */
        remove_proc_entry("emerald", NULL /* parent dir */);
}

#endif

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
/* Don't add __exit macro to the declaration of this cleanup function
 * since it is also called at init time, if init fails. */
static void emerald_cleanup_module(void)
{
        int i;
        /* cleanup_module is never called if registering failed */
        unregister_chrdev(emerald_major, "emerald");
                                                                                
#ifdef EMERALD_DEBUG /* use proc only if debugging */
        emerald_remove_proc();
#endif

        if (emerald_ports) {
                kfree(emerald_ports);
        }

        if (emerald_boards) {
                for (i=0; i<emerald_nr_ok; i++) {
                        if (emerald_boards[i].region) 
                            release_region(emerald_boards[i].ioport,EMERALD_IO_REGION_SIZE);
                }
                kfree(emerald_boards);
        }

}

static int emerald_open (struct inode *inode, struct file *filp)
{
        int num = MINOR(inode->i_rdev);
        emerald_port *port; /* device information */

        /* Inform kernel that this device is not seekable */
        nonseekable_open(inode,filp);

        /*  check the device number */
        if (num >= emerald_nports) return -ENODEV;
        port = emerald_ports + num;

        /* and use filp->private_data to point to the device data */
        filp->private_data = port;

        // Don't access the ioport/irq configuration registers here.
        // It may interfere with simultaneous serial port accesses.
        return 0;
}

static int emerald_release (struct inode *inode, struct file *filp)
{
        return 0;
}

/*
 * The ioctl() implementation
 */

static int emerald_ioctl (struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
        emerald_port* port = filp->private_data;
        emerald_board* brd = port->board;
        int err= 0, ret = 0;

        if (_IOC_TYPE(cmd) != EMERALD_IOC_MAGIC) return -ENOTTY;
        if (_IOC_NR(cmd) > EMERALD_IOC_MAXNR) return -ENOTTY;

        /*
         * the type is a bitmask, and VERIFY_WRITE catches R/W
         * transfers. Note that the type is user-oriented, while
         * verify_area is kernel-oriented, so the concept of "read" and
         * "write" is reversed
         */
        if (_IOC_DIR(cmd) & _IOC_READ)
                err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
        else if (_IOC_DIR(cmd) & _IOC_WRITE)
                err =  !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
        if (err) return -EFAULT;

        switch(cmd) {
        case EMERALD_IOCGPORTCONFIG:	/* get current port config */
                if (copy_to_user((emerald_config *) arg,&brd->config,
                        sizeof(emerald_config)) != 0) ret = -EFAULT;
                break;
        case EMERALD_IOCSPORTCONFIG:	/* set port config in registers */
                /* Warning: interferes with concurrent serial driver operations on the ports.
                 * Can cause system crash if serial driver is accessing tty ports. */
                {
                        emerald_config tmpconfig;
                        if (copy_from_user(&tmpconfig,(emerald_config *) arg,
                            sizeof(emerald_config)) != 0) ret = -EFAULT;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        ret = emerald_write_config(brd,&tmpconfig);
                        // read the ioport values back
                        if (!ret) ret = emerald_read_config(brd);
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCGEEPORTCONFIG:	/* get config from eeprom */
                {
                        emerald_config eeconfig;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        ret = emerald_read_eeconfig(brd,&eeconfig);
                        mutex_unlock(&brd->brd_mutex);
                        if (copy_to_user((emerald_config *) arg,&eeconfig,
                                sizeof(emerald_config)) != 0) ret = -EFAULT;
                }
                break;
        case EMERALD_IOCSEEPORTCONFIG:	/* set config in eeprom. User should then load it */
                {
                        emerald_config eeconfig;
                        if (copy_from_user(&eeconfig,(emerald_config *) arg,
                            sizeof(emerald_config)) != 0) ret = -EFAULT;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        ret = emerald_write_eeconfig(brd,&eeconfig);
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCEECONFIGLOAD:	/* load EEPROM config */
                /* Warning: interferes with concurrent serial driver operations on the ports.
                 * Can cause system crash is serial driver is accessing tty ports. */
                {
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        ret = emerald_load_config_from_eeprom(brd);
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCPORTENABLE:
                /* Warning: interferes with concurrent serial driver operations on the ports.
                 * Can cause system crash is serial driver is accessing tty ports. */
                {
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        emerald_enable_ports(brd);
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCGNBOARD:    /* how many boards are responding at the given ioport addresses */
                if (copy_to_user((int*) arg,&emerald_nr_ok,
                        sizeof(int)) != 0) ret = -EFAULT;
                break;
        case EMERALD_IOCGISABASE:   /* what is the base ISA address on this system */
                if (copy_to_user((unsigned long*) arg,&ioport_base,
                        sizeof(unsigned long)) != 0) ret = -EFAULT;
                break;
        case EMERALD_IOCGDIOOUT:    /* get digio direction for a port, 1=out, 0=in */
                {
                        int iport = port->portNum;
                        int val;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        val = emerald_get_digio_port_out(brd,iport);
                        if (copy_to_user((int*) arg,&val,
                                sizeof(int)) != 0) ret = -EFAULT;
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCSDIOOUT:    /* set digio direction for a port, 1=out, 0=in */
                {
                        int iport = port->portNum;
                        int val = (int) arg;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        emerald_set_digio_port_out(brd,iport,val);
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCGDIO:   /* get digio value for a port */
                {
                        int iport = port->portNum;
                        int val;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        val = emerald_read_digio_port(brd,iport);
                        if (copy_to_user((int*) arg,&val,
                                sizeof(int)) != 0) ret = -EFAULT;
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        case EMERALD_IOCSDIO:   /* set digio value for a port */
                {
                        int iport = port->portNum;
                        int val = (int) arg;

                        // digio line must be an output
                        if (! (brd->digioout & (1 << iport))) return -EINVAL;
                        if ((ret = mutex_lock_interruptible(&brd->brd_mutex))) return ret;
                        emerald_write_digio_port(brd,iport,val);
                        mutex_unlock(&brd->brd_mutex);
                }
                break;
        default:  /* redundant, as cmd was checked against MAXNR */
                return -ENOTTY;
        }
        return ret;
}

static struct file_operations emerald_fops = {
        .owner   = THIS_MODULE,
        .ioctl   = emerald_ioctl,
        .open    = emerald_open,
        .release = emerald_release,
        .llseek  = no_llseek,
};
						      
static int __init emerald_init_module(void)
{
        int result, ib,ip;
        emerald_board* ebrd;

        /*
         * Register your major, and accept a dynamic number. This is the
         * first thing to do, in order to avoid releasing other module's
         * fops in emerald_cleanup_module()
         */
        result = register_chrdev(emerald_major, "emerald", &emerald_fops);
        if (result < 0) {
                printk(KERN_WARNING "emerald: can't get major %d\n",emerald_major);
                return result;
        }
        if (emerald_major == 0) emerald_major = result; /* dynamic */
        PDEBUGG("major=%d\n",emerald_major);

        for (ib=0; ib < EMERALD_MAX_NR_DEVS; ib++)
                if (ioports[ib] == 0) break;
        emerald_nr_addrs = ib;
        PDEBUGG("nr_addrs=%d\n",emerald_nr_addrs);

        /*
         * allocate the board structures
         */
        emerald_boards = kmalloc(emerald_nr_addrs * sizeof(emerald_board), GFP_KERNEL);
        if (!emerald_boards) {
                result = -ENOMEM;
                goto fail;
        }
        memset(emerald_boards, 0, emerald_nr_addrs * sizeof(emerald_board));

        ebrd = emerald_boards;
        for (ib=0; ib < emerald_nr_addrs; ib++) {
                int boardOK = 0;
                // If a board doesn't respond we reuse this structure space,
                // so zero it again
                memset(ebrd, 0, sizeof(emerald_board));
                ebrd->ioport = ioports[ib] + ioport_base;
                // printk(KERN_INFO "emerald: addr=0x%lx\n",ebrd->ioport);
                if (!(ebrd->region =
                        request_region(ebrd->ioport,EMERALD_IO_REGION_SIZE,
                                    "emerald"))) {
                        result = -EBUSY;
                        goto fail;
                }
                mutex_init(&ebrd->brd_mutex);

                /*
                 * Read ioport and irq configuration from EEPROM and see if
                 * it looks OK.  emerald_nr_ok will be the number of boards
                 * that are configured correctly or are configurable.
                 */
                result = emerald_read_eeconfig(ebrd,&ebrd->config);
                if (result == -ENODEV) {
                        release_region(ebrd->ioport,EMERALD_IO_REGION_SIZE);
                        continue;
                }

                if (result) {
                        /*
                         * We have seen situations where the EMM-8P EEPROM is not
                         * accessible, which appeared to be due to a +0.4 V
                         * over-voltage on the 5V PC104 power supply. (This doesn't effect
                         * an EMM-8M). When the EEPROM accesses fail here, we try
                         * to read the register values with emerald_read_config,
                         * since they should have been initialized from EEPROM at boot.
                         * However it appears that the boot initialization must have
                         * failed too, since the register values are all zeroes.
                         * In this case we initialize the register values with
                         * some defaults, and proceed.
                         */
                        printk(KERN_INFO "emerald: failure reading config from eeprom on board at ioports[%d]=0x%x\n",ib,ioports[ib]);
                        printk(KERN_INFO "emerald: reading config from registers\n");
                        result = emerald_read_config(ebrd);     // try anyway
                }
                if (!result && emerald_check_config(&ebrd->config)) boardOK = 1;
                else {
                        emerald_config tmpconfig;
                        printk(KERN_INFO "emerald: invalid config on board at ioports[%d]=0x%x, ioport[0]=0x%x, irq[0]=%d\n",ib,ioports[ib],
                            ebrd->config.ports[0].ioport,ebrd->config.ports[0].irq);
                        // write a default configuration to registers and check
                        // to see if it worked.
                        for (ip = 0; ip < EMERALD_NR_PORTS; ip++) {
                                tmpconfig.ports[ip].ioport = 0x100 + (emerald_nr_ok * EMERALD_NR_PORTS + ip) * 8;
                                tmpconfig.ports[ip].irq = 3;
                        }
                        emerald_write_config(ebrd,&tmpconfig);
                        result = emerald_read_config(ebrd);
                        if (!result && emerald_check_config(&ebrd->config)) {
                                printk(KERN_INFO "emerald: valid config written to registers on board at ioports[%d]=0x%x\n",ib,ioports[ib]);
                                boardOK = 1;
                        }
                        else printk(KERN_INFO "emerald: cannot write valid config to registers on board at ioports[%d]=0x%x\n",ib,ioports[ib]);
                }
                
                if (boardOK) {
                        emerald_nr_ok++;
                        emerald_set_digio_out(ebrd,0);
                        emerald_read_digio(ebrd);
                        ebrd++;
                }
                else release_region(ebrd->ioport,EMERALD_IO_REGION_SIZE);
        }
        printk(KERN_INFO "emerald: %d boards found\n",emerald_nr_ok);
        if (emerald_nr_ok == 0 && result != 0) goto fail;

        emerald_nports = emerald_nr_ok * EMERALD_NR_PORTS;
        emerald_ports = kmalloc(emerald_nports * sizeof(emerald_port), GFP_KERNEL);
        if (!emerald_ports) {
                result = -ENOMEM;
                goto fail;
        }
        memset(emerald_ports, 0,emerald_nports * sizeof(emerald_port));

        for (ip=0; ip < emerald_nports; ip++) {
                emerald_port* eport = emerald_ports + ip;
                emerald_board* ebrd = emerald_boards + (ip / EMERALD_NR_PORTS);
                eport->board = ebrd;
                eport->portNum = ip % EMERALD_NR_PORTS;	// 0-7
        }

#ifdef EMERALD_DEBUG /* only when debugging */
        PDEBUGG("create_proc\n");
        emerald_create_proc();
#endif
        return 0; /* succeed */

fail:
        emerald_cleanup_module();
        return result;
}

module_init(emerald_init_module);
module_exit(emerald_cleanup_module);
