#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include "sw_spi.h"
#include "lib_soft_spi.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "soft_spi"
#define CLASS_NAME "softspicla"

#define DEBUG_MOD

struct mod_ctrl_s
{
    unsigned occupied_flag;
    pid_t    pid;
};

//#define DEBUG_MOD

static int major_num;
static struct class* cl;
static struct device* dev;

struct mod_ctrl_s mod_ctrl;
size_t len;     // size of the data will be transfered
void* buf = NULL; // buffer

unsigned mosi_pin;
unsigned miso_pin;
unsigned sck_pin;
unsigned cs_pin;
unsigned cpol;
unsigned cpha;

extern unsigned data_xchg_flag;
extern unsigned cs_ctrl_man_flag;

// Determine a lock for the spinlock,
// It is needed for prenventing the preemption when 
// generating the scl and sda signal.
spinlock_t wire_lock;

static int __init soft_spi_bus_driver_init(void);
static void __exit soft_spi_bus_driver_exit(void);
static int soft_spi_bus_driver_probe(struct platform_device* dev);
static int soft_spi_bus_driver_remove(struct platform_device* dev);


static int soft_spi_bus_open(struct inode* node, struct file* file);
static int soft_spi_bus_release(struct inode*, struct file* );
static loff_t soft_spi_bus_lseek(struct file*, loff_t, int);
static ssize_t soft_spi_bus_read(struct file*, char* ,size_t,loff_t*);
static ssize_t soft_spi_bus_write(struct file*, const char*, size_t, loff_t*);
long soft_spi_bus_ioctl (struct file *, unsigned int, unsigned long);

static struct file_operations file_ops = {
    .llseek  = soft_spi_bus_lseek, 
    .read    = soft_spi_bus_read,
    .write   = soft_spi_bus_write,
    .unlocked_ioctl = soft_spi_bus_ioctl,
    .open    = soft_spi_bus_open,
    .release = soft_spi_bus_release
};

static const struct of_device_id soft_spi_bus_match[] = {
    {.compatible = "soft_spi_bus"},
    {}
};

struct platform_driver soft_spi_bus_drv = {
    .probe = soft_spi_bus_driver_probe,
    .remove = soft_spi_bus_driver_remove,
    .driver = {
        .name = "soft_spi_bus",
        .of_match_table = soft_spi_bus_match,
    }
};

static int soft_spi_bus_driver_probe(struct platform_device* pdev)
{
    struct device_node* np = pdev->dev.of_node;

    printk(KERN_INFO "[soft_spi_drv] : probe starting\n");

    //Gettng the sck pin
    if(of_property_read_u32(np, "sck_pin", &sck_pin)){
        goto spi_sck_failed_l;
    }

    // Getting the miso pin
    if(of_property_read_u32(np, "miso_pin", &miso_pin)){
        goto spi_sck_failed_l;
    }

    // Getting the mosi pin
    if(of_property_read_u32(np, "mosi_pin", &mosi_pin)){
        goto spi_sck_failed_l;
    }

    printk(KERN_INFO "[soft_spi_drv] : sck %u, mosi %u, miso %u\n", sck_pin, mosi_pin, miso_pin);

    // Initialize the soft iic bus
    if(0 != spi_sck_request(sck_pin)){
        goto spi_sck_failed_l;
    }

    if(0 != spi_miso_request(miso_pin)){
        goto spi_miso_failed_l;
    }

    if(0 != spi_mosi_request(mosi_pin)){
        goto spi_mosi_failed_l;
    }

    mod_ctrl.occupied_flag = 0;
    mod_ctrl.pid = 0;

    spin_lock_init(&wire_lock);

    // device_create(...,"hello"), the hello here might means after this function, the name for this driver under /dev/ is hello,
    // so we will have /dev/hello
    dev = device_create(cl,NULL,MKDEV(major_num,0),NULL,DEVICE_NAME);
    if(IS_ERR(dev)){
        printk(KERN_ALERT "[sw_spi] failed device create\n");
        goto device_create_failed_l;
    }

    printk(KERN_INFO "[sw_spi_drv] probe starting done\n");

    return 0;

device_create_failed_l:
    spi_mosi_free();
spi_mosi_failed_l:
    spi_miso_free();
spi_miso_failed_l:
    spi_sck_free();
spi_sck_failed_l:
    return -1;
}

static int soft_spi_bus_driver_remove(struct platform_device* dev)
{
    spi_mosi_free();
    spi_miso_free();
    spi_sck_free();

    device_destroy(cl, MKDEV(major_num,0));
    
    return 0;
}

static int __init soft_spi_bus_driver_init(void)
{
    printk(KERN_INFO "[sw_spi] softspi: staring...\n");

    // Find a place in kernel char-device-array for keeping and allocate major-id for this driver.
    // major-id is the index for this driver in char-device-array.
    // After this step, we can find this device from /proc/devices
    major_num = register_chrdev(0,DEVICE_NAME,&file_ops);
    if(0 > major_num){
    printk(KERN_ALERT "[sw_spi] failed alloc\n");
        goto finish_l;
    }

    // Create a class under /sys/class, so that later we can call device_create() to create a node under /dev/
    cl = class_create(THIS_MODULE,CLASS_NAME);
    if(IS_ERR(cl)){
        printk(KERN_ALERT "[sw_spi] failed create class\n");
        goto unregister_chrdev_region_l;
    } 

    if(NULL == (buf = vmalloc(4096))){
        goto class_destroy_l;
    }

    if(platform_driver_register(&soft_spi_bus_drv)){
        goto platform_driver_register_failed_l;
    }

    printk(KERN_INFO "[sw_spi] staring done.\n");
    return 0;

platform_driver_register_failed_l:
    vfree(buf);
class_destroy_l:
    class_destroy(cl);
unregister_chrdev_region_l:
    unregister_chrdev(major_num, DEVICE_NAME);
finish_l:
    return -1;
}

static void __exit soft_spi_bus_driver_exit(void)
{
    printk(KERN_INFO "[sw_spi]: stopping...\n");

    platform_driver_unregister(&soft_spi_bus_drv);
    vfree(buf);
    class_unregister(cl);
    class_destroy(cl);
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "[sw_spi]: stopping done.\n");
}

static int soft_spi_bus_open(struct inode* inode, struct file* file)
{
    printk(KERN_INFO " %d device openning, mosi %u miso %u sck %u\n", 
                       current->pid,       mosi_pin,miso_pin,sck_pin);

    if(mod_ctrl.occupied_flag){
        return -ENOSYS;
    }

    return 0;
}

static int soft_spi_bus_release(struct inode* node, struct file* file)
{
    printk(KERN_INFO "%d device released\n", current->pid);

    spi_cs_free();

    if(mod_ctrl.occupied_flag && (mod_ctrl.pid == current->pid)){
        mod_ctrl.occupied_flag = 0;
    }

    return 0;
}

long soft_spi_bus_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = -ENOSYS;

    switch(cmd)
    {
        case IOCTL_CMD_TAKE:
            spin_lock_irq(&wire_lock);
            if(!mod_ctrl.occupied_flag){
                if(spi_cs_request(arg)){
                    spin_unlock_irq(&wire_lock);
                    break;
                }
                soft_spi_init();
                mod_ctrl.occupied_flag = 1;
                mod_ctrl.pid = current->pid;
                ret = 0;
            }
            spin_unlock_irq(&wire_lock);
        break;

        case IOCTL_CMD_RELEASE:
            spin_lock_irq(&wire_lock);
            if(mod_ctrl.occupied_flag && (mod_ctrl.pid == current->pid)){
                spi_cs_free();
                mod_ctrl.occupied_flag = 0;
                ret = 0;
            }
            spin_unlock_irq(&wire_lock);
        break;
        case IOCTL_CMD_CS_CTRL_MAN:
            cs_ctrl_man_flag = 1;
        break;
        case IOCTL_CMD_CS_CTRL_AUTO:
            cs_ctrl_man_flag = 0;
        break;
        case IOCTL_CMD_CS_CTRL:
            soft_spi_cs_set(arg);
        break;
        case IOCTL_CMD_DATA_XCHG:
            data_xchg_flag = 1;
        break;
        case IOCTL_CMD_DATA_READ:
            data_xchg_flag = 0;
        break;
        default:
            return EPERM;
    }

    return ret;
}


static loff_t soft_spi_bus_lseek(struct file* file, loff_t offset, int orig)
{
    if(mod_ctrl.occupied_flag && (mod_ctrl.pid != current->pid)){
        return -1;
    }

    printk(KERN_INFO "[sw_spi] %d lseek %ld %llx\n",current->pid, sizeof(loff_t), offset);

    return 0;
}

/**
 * size => len
 */
static ssize_t soft_spi_bus_read(struct file* file, char* str,size_t size,loff_t* offset)
{ 
    len = size;
    ((uint8_t*)buf)[0] = 0;
    
    if(mod_ctrl.occupied_flag && (mod_ctrl.pid != current->pid)){
        return -1;
    }

    if(data_xchg_flag){
        copy_from_user(buf, str, len);
    }

    if(soft_spi_read(buf, len)){
        printk(KERN_INFO "[sw_spi] no ack!!\n");
        return -1;
    }

#ifdef DEBUG_MOD
    int i;
    printk(KERN_INFO "[sw_spi] read : ");
    for (i = 0; i < len; i++)
    {
        printk(KERN_INFO "%d,",((char*)buf)[i]);
    }
    printk(KERN_INFO "\n");
#endif

    copy_to_user(str, buf, len);
    
    return 0;
}

/**
 *
 */
static ssize_t soft_spi_bus_write(struct file* file, const char* str, size_t size, loff_t* offset)
{    
    len = size;
    
    if((!mod_ctrl.occupied_flag) || (mod_ctrl.pid != current->pid)){
        return -1;
    }
    
    copy_from_user(buf,str,size);

    printk(KERN_INFO "[sw_spi] %d Going to write\n",current->pid);

#ifdef DEBUG_MOD
    int i;
    for (i = 0  ; i < len ; i++)
    {
        printk(KERN_INFO "%d,",((char*)buf)[i]);
    }
    printk(KERN_INFO "\n");
#endif

    if(soft_spi_write(buf, len)){
        printk(KERN_INFO "no ack from slave\n");
        return -1;
    }
    
    printk(KERN_INFO "write done\n");
    
    return 0;
}

module_init(soft_spi_bus_driver_init);
module_exit(soft_spi_bus_driver_exit);
