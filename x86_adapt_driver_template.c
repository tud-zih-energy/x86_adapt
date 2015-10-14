#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mount.h>
#include <asm/uaccess.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/version.h>

#include <linux/sched.h>

#include <asm/amd_nb.h>

#define MODULE_NAME "x86_adapt"

/* allocates dynamicly a char device region, intializes it and adds it */
#define ALLOC_AND_INIT(DEV, FUNC, TYPE) \
    do { \
        err = alloc_chrdev_region(&(DEV##_device),0,num_possible_##TYPE##s()+1,#DEV); \
        if (err) { \
            printk(KERN_ERR "Failed to allocate chrdev_region for %s\n", #DEV); \
            goto fail; \
        } else { \
            DEV##_device_ok = 1 ; \
        } \
        FUNC(DEV, TYPE); \
        init_rwsem(&(DEV##_cdev.rwsem));\
        cdev_init(&(DEV##_cdev.cdev), & DEV##_fops); \
        DEV##_cdev.cdev.owner = THIS_MODULE; \
        err = cdev_add(&(DEV##_cdev.cdev), DEV##_device, num_possible_##TYPE##s()+1); \
        if (err) { \
            printk(KERN_ERR "Failed to add cdev for %s\n", #DEV); \
            goto fail; \
        } else { \
            DEV##_cdev_ok = 1 ; \
        } \
    } while (0)

/* allocates memory for a northbridge pci device and adds it */
#define ALLOC_UNCORE_PCI(NAME, NUM, NUM2) \
    do { \
        NAME = kmalloc(num_possible_nodes()*sizeof(struct pci_dev *),GFP_KERNEL); \
        if (NAME == NULL) { \
            err = -ENOMEM; \
            printk(KERN_ERR "Failed to allocate memory for %d %d\n", NUM,NUM2); \
            goto fail; \
        } \
        for_each_online_node(i) \
            NAME [i] = pci_get_bus_and_slot(get_uncore_bus_id(i), PCI_DEVFN(NUM,NUM2)); \
    } while (0)



/* allocates memory for a northbridge pci device and adds it */
#define ALLOC_NB_PCI(NUM) \
    do { \
        nb_f##NUM = kmalloc(num_possible_nodes()*sizeof(struct pci_dev *),GFP_KERNEL); \
        if (nb_f##NUM == NULL) { \
            err = -ENOMEM; \
            printk(KERN_ERR "Failed to allocate memory for nb_fd%d\n", NUM); \
            goto fail; \
        } \
        for_each_online_node(i) \
            if (node_to_amd_nb(i)!=NULL) \
              nb_f##NUM [i] = pci_get_bus_and_slot(0, PCI_DEVFN(PCI_SLOT(node_to_amd_nb(i)->misc->devfn),NUM)); \
            else \
              nb_f##NUM [i] = NULL; \
    } while (0)

/* checks if the char device was properly intialized and then it will be unregistered and deleted */
#define UNREGISTER_AND_DELETE(DEV, FUNC, TYPE) \
    do { \
        if (DEV##_cdev_ok) \
            cdev_del(&(DEV##_cdev.cdev)); \
        FUNC(DEV, TYPE); \
        if (DEV##_device_ok) \
            unregister_chrdev_region(DEV##_device, num_possible_##TYPE##s()+1); \
    } while (0)

/* frees the allocated space for this pci device */
#define FREE_PCI(NAME) \
    do { \
        if (NAME) { \
            for_each_node(i) { \
                if (NAME [i]) \
                    pci_dev_put (NAME [i]); \
            } \
            kfree(NAME); \
            NAME = NULL; \
        } \
    } while (0)

/* frees the allocated space for this pci device */
#define FREE_NB(NUM) \
    do { \
        if (nb_f##NUM) { \
            for_each_node(i) { \
                if (nb_f##NUM [i]) \
                    pci_dev_put (nb_f##NUM [i]); \
            } \
            kfree(nb_f##NUM); \
            nb_f##NUM = NULL; \
        } \
    } while (0)

/* adds all online devices */
#define DEVICE_CREATE(DEV, TYPE) \
    do { \
        struct device *dev; \
        for_each_online_##TYPE(i) { \
            dev = device_create(x86_adapt_class,NULL,MKDEV(MAJOR(DEV##_device), \
                MINOR(DEV##_device)+i),NULL,"%s_%d",#TYPE,i); \
            if (IS_ERR(dev)) { \
                    printk(KERN_ERR "Failed to create device /sys/class/x86_adapt%s%d\n",#TYPE,i); \
                    err = PTR_ERR(dev); \
                    goto fail; \
            } \
        } \
        dev = device_create(x86_adapt_class,NULL,MKDEV(MAJOR(DEV##_device), \
            MINOR(DEV##_device)+num_possible_##TYPE##s()),NULL,"%s_all",#TYPE); \
        if (IS_ERR(dev)) { \
            printk(KERN_ERR "Failed to create device /sys/class/x86_adapt/%s_all\n",#TYPE); \
            err = PTR_ERR(dev); \
            goto fail; \
        } \
    } while (0)

/* destroys all online devices */
#define DEVICE_DESTROY(DEV, TYPE) \
    do { \
        for_each_online_##TYPE(i) { \
            device_destroy(x86_adapt_class,MKDEV(MAJOR(DEV##_device), \
                MINOR(DEV##_device)+i)); \
        } \
            device_destroy(x86_adapt_class,MKDEV(MAJOR(DEV##_device), \
                MINOR(DEV##_device)+num_possible_##TYPE##s())); \
    } while (0)

/* create definition devices */
#define DEF_CREATE(DEV, TYPE) \
    do { \
        struct device *dev; \
        dev = device_create(x86_adapt_class,NULL,DEV##_device,NULL, \
            "%s_definitions",#TYPE);  \
        if (IS_ERR(dev)) { \
            printk(KERN_ERR "Failed to create device /sys/class/x86_adapt/%s_definitions\n",#TYPE); \
            err = PTR_ERR(dev); \
            goto fail; \
        } \
    } while (0)
                
/* destroy definition devices */
#define DEF_DESTROY(DEV, TYPE) \
    device_destroy(x86_adapt_class,DEV##_device)
    
#define X86_ADAPT_CPU 0
#define X86_ADAPT_NODE 1


struct x86_adapt_cdev {
    struct cdev cdev;
    struct rw_semaphore rwsem;
};

/* character devices */
static dev_t x86_adapt_cpu_device;
static dev_t x86_adapt_node_device;
static dev_t x86_adapt_def_cpu_device;
static dev_t x86_adapt_def_node_device;
static struct x86_adapt_cdev x86_adapt_cpu_cdev;
static struct x86_adapt_cdev x86_adapt_node_cdev;
static struct x86_adapt_cdev x86_adapt_def_cpu_cdev;
static struct x86_adapt_cdev x86_adapt_def_node_cdev;
/* initialize check variables */
static int x86_adapt_cpu_device_ok = 0;
static int x86_adapt_node_device_ok = 0;
static int x86_adapt_def_cpu_device_ok = 0;
static int x86_adapt_def_node_device_ok = 0;
static int x86_adapt_cpu_cdev_ok = 0;
static int x86_adapt_node_cdev_ok = 0;
static int x86_adapt_def_cpu_cdev_ok = 0;
static int x86_adapt_def_node_cdev_ok = 0;


/* temporary */
static struct class * x86_adapt_class;

/* everything I could think of :P */
enum{
    MSR = 0,
    NB_F0, /* Device 15, Function 0 */
    NB_F1, /* Device 15, Function 1 */
    NB_F2, /* Device 15, Function 2 */
    NB_F3, /* Device 15, Function 3 */
    NB_F4, /* Device 15, Function 4 */
    NB_F5, /* Device 15, Function 5 */
    SB_PCU0, /* Device 10, Function 0 */
    SB_PCU1, /* Device 10, Function 1 */
    SB_PCU2, /* Device 10, Function 2 */
    HSW_PCU0, /* Device 30, Function 0 */
    HSW_PCU1, /* Device 30, Function 1 */
    HSW_PCU2, /* Device 30, Function 2 */
    UNCORE,
};

/* 4 byte id + 1 byte length + 4 byte name length + 4 byte description length */
#define ENTRY_HEADER_SIZE 13 

struct knob_entry {
    /* user */
    char * name;
    char * description;
    u32 id;
    u8 length;
    /* internal */
    u8 device;
    u8 readonly;
    u64 register_index;
    u64 bitmask;
    u32 restricted_settings_length;
    u32 reserved_settings_length;
    u64 * restricted_settings;
    u64 * reserved_settings;
};

struct fam_struct {
    u16 family;
    u8 model_length;
    u16 * models; 
};

struct knob_vendor {
    u8 vendor;
    u8 features_length;
    u64 * features;
    u8 fam_length;
    struct fam_struct * fams;
};

struct knob_entry_definition {
    struct knob_entry knob;
    u16 av_length;
    struct knob_vendor ** av_vendors;
};

/* TODO: refactor, so that the AMD devices dont use memory on Intel and vice versa */

/* AMD */
struct pci_dev ** nb_f0 = NULL;
struct pci_dev ** nb_f1 = NULL;
struct pci_dev ** nb_f2 = NULL;
struct pci_dev ** nb_f3 = NULL;
struct pci_dev ** nb_f4 = NULL;
struct pci_dev ** nb_f5 = NULL;

/* Intel Sandy Bridge EP */
struct pci_dev ** sb_pcu0 = NULL;
struct pci_dev ** sb_pcu1 = NULL;
struct pci_dev ** sb_pcu2 = NULL;

/* Intel Haswell EP */
struct pci_dev ** hsw_pcu0 = NULL;
struct pci_dev ** hsw_pcu1 = NULL;
struct pci_dev ** hsw_pcu2 = NULL;


/* here should be all the information inserted by python */

#template_holder

/* here should be all the information inserted by python */


/* This function gives you the bus id for uncore components of a NUMA node in sandy bridge ep processors */
static int get_uncore_bus_id(int node_id)
{
  if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL){
    struct pci_dev *ubox_dev = NULL;
    u32 config = 0;
    int devid = 0;

    /* dev id based on processor */

    /* based on arch/x86/kernel/cpu/perf_event_intel_uncore.c for Sandy Bridge / IvyTown */
    /* based on Datasheet 2 for Haswell EP */
    /* Sandy Bridge   : 0x3ce0 */
    /* Ivy Town:        0x0e1e*/
    /* Haswell EP:        0x2f1e*/
    switch (boot_cpu_data.x86_model) {
      /* Sandy Bridge EP: 0x3ce0 */
      case 45: devid = 0x3ce0; break;
      case 62: devid = 0x0e1e; break;
      case 0x3f: devid = 0x2f1e; break;
      default:
        // printk(KERN_ERR "Unsupported Intel processor for Uncore stuff.\n");
        return -1;
        /* keep it at 0 */
    }

    while (1) {
      /* find the UBOX device */
      int bus;
      int nodeid;
      int err;
      ubox_dev = pci_get_device(PCI_VENDOR_ID_INTEL, devid, ubox_dev);
      if (!ubox_dev){
        printk(KERN_ERR MODULE_NAME"Could not get Intel Uncore device - missing ubox.\n");
        return -1;
      }
      bus = ubox_dev->bus->number;
      /* get the Node ID of the local register */
      err = pci_read_config_dword(ubox_dev, 0x40, &config);
      if (err){
        printk(KERN_ERR MODULE_NAME"Could not get Intel node id via pci %d.\n", err);
        return -1;
      }
      nodeid = config;
      if (nodeid == node_id){
        pci_dev_put(ubox_dev);
        /* printk (KERN_INFO "Given Node: %d, Found Node: %d, Bus: %d\n",node_id,nodeid, bus); */
        return bus;
      }
    }
    if (ubox_dev)
      pci_dev_put(ubox_dev);
    printk(KERN_ERR MODULE_NAME"This should not occur.\n");
    /* Should not occur */
    return -1;
  }
  /* Unsupported processor */
  return -1;

}

static u32 active_knobs_node_length = 0;
static struct knob_entry * active_knobs_node = NULL;

static u32 active_knobs_cpu_length = 0;
static struct knob_entry * active_knobs_cpu = NULL;

static void increment_knob_counter(u32 i, u32 *nr_knobs_cpu, u32 *nr_knobs_node) 
{
    if (all_knobs[i].knob.device == MSR) 
        (*nr_knobs_cpu)++; 
    else 
        (*nr_knobs_node)++; 
    return;
}

static void add_knob(u32 i, u32 *nr_knobs_cpu, u32 *nr_knobs_node) 
{
    if (all_knobs[i].knob.device == MSR) { 
        memcpy(&active_knobs_cpu[*nr_knobs_cpu],&(all_knobs[i].knob),
            sizeof (struct knob_entry)); 
        //printk(KERN_INFO "Added cpu feature %s\n", active_knobs_cpu[*nr_knobs_cpu].name);
        (*nr_knobs_cpu)++; 
    } else { 
        memcpy(&active_knobs_node[*nr_knobs_node],&(all_knobs[i].knob),
            sizeof (struct knob_entry)); 
        //printk(KERN_INFO "Added node feature %s\n", active_knobs_node[*nr_knobs_node].name);
        (*nr_knobs_node)++; 
    } 
    return;
}

/* traverses all knobs and either counts the avaible knobs or adds the avaible knobs */
static void traverse_knobs(u32 *nr_knobs_cpu, u32 *nr_knobs_node,
    void (*do_knob)(u32, u32*, u32*)) 
{
    u32 i,j,k,l,has_features;
    struct cpuinfo_x86 info = cpu_data(0);
    u8 vendor = info.x86_vendor;
    u8 family = info.x86;
    u8 model = info.x86_model;

    /* for all knobs do something */
    for (i = 0;i<all_knobs_length;i++) {
      //printk("Check :%s\n",all_knobs[i].knob.name);
        for (j = 0;j<all_knobs[i].av_length;j++) {
            /* check vendor */
            if (vendor != all_knobs[i].av_vendors[j]->vendor)
                continue;
            /* check whether it has all the features wanted */
            has_features = 1;
            for (k = 0;k<all_knobs[i].av_vendors[j]->features_length;k++)
                if (!boot_cpu_has(all_knobs[i].av_vendors[j]->features[k]))
                    has_features = 0;
            if (!has_features)
                continue;
            if (all_knobs[i].av_vendors[j]->fam_length == 0) 
                do_knob(i, nr_knobs_cpu, nr_knobs_node);

            for (k = 0;k<all_knobs[i].av_vendors[j]->fam_length;k++) {
                /* check family */
                if (family != all_knobs[i].av_vendors[j]->fams[k].family)
                    continue;
                if (all_knobs[i].av_vendors[j]->fams[k].model_length == 0)
                    do_knob(i, nr_knobs_cpu, nr_knobs_node);
                else 
                    for (l = 0;l<all_knobs[i].av_vendors[j]->fams[k].model_length;l++)
                        /* check model */
                        if (model == all_knobs[i].av_vendors[j]->fams[k].models[l]) 
                            do_knob(i, nr_knobs_cpu, nr_knobs_node);
            }
        }
    }
    return;
}

/* used at init */
static int buildup_entries(void) 
{
    u32 nr_knobs_cpu = 0,nr_knobs_node = 0;

    /* count number of knobs */
    traverse_knobs(&nr_knobs_cpu, &nr_knobs_node, increment_knob_counter);
    // printk("Found %d cpu features\n", nr_knobs_cpu);
    // printk("Found %d node features\n", nr_knobs_node);

    /* allocate memory for the avaible knobs */
    active_knobs_cpu = kmalloc(nr_knobs_cpu*sizeof(struct knob_entry),GFP_KERNEL);
    if (unlikely(active_knobs_cpu == NULL))
        return -ENOMEM;
    active_knobs_cpu_length = nr_knobs_cpu;
    active_knobs_node = kmalloc(nr_knobs_node*sizeof(struct knob_entry),GFP_KERNEL);
    if (unlikely(active_knobs_node == NULL))
        return -ENOMEM;
    active_knobs_node_length = nr_knobs_node;

    /* add knobs */
    nr_knobs_cpu = 0;
    nr_knobs_node = 0;
    traverse_knobs(&nr_knobs_cpu, &nr_knobs_node, add_knob);

    return 0;
}

/* overrides the default kernel llseek function to prevent a not working pread/pwrite on certain kernel versions */
static loff_t x86_adapt_seek(struct file *file, loff_t offset, int orig)
{
	loff_t ret;
	struct inode *inode = file->f_mapping->host;

	mutex_lock(&inode->i_mutex);
	switch (orig) {
	case 0:
		file->f_pos = offset;
		ret = file->f_pos;
		break;
	case 1:
		file->f_pos += offset;
		ret = file->f_pos;
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&inode->i_mutex);
	return ret;
}


/* returns from register reading the corresponding bits to the given bitmask */
__always_inline static u64 get_setting_from_register_reading(u64 register_reading, u64 bitmask) 
{
    int i;
    u64 ret_val = 0;
    ret_val = bitmask & register_reading;
    for (i = 0;i<64;i++) {
        if (bitmask&(1ULL<<i)) {
            break;
        }
    }
    if (i == 64)
        return 0;
    ret_val = ret_val>>i;
    return ret_val;
}

/* reads the setting of msr / pci knob */
static u64 read_setting(int dev_nr, struct knob_entry knob) 
{
    u64 register_reading = 0;
    u32 l = 0,h;
    /* switch device */
    if (dev_nr >= 0) {
        struct pci_dev * nb = NULL;
        switch (knob.device) {
            case MSR:
                if ( cpumask_equal(get_cpu_mask(dev_nr),&(current->cpus_allowed))) {
                    register_reading = native_read_msr(knob.register_index);
                }
                else {
                    rdmsr_on_cpu(dev_nr, knob.register_index,&l, &h);
                    register_reading = (l|(((u64)h)<<32));
                }
                break;
            case NB_F0:
                nb = nb_f0[dev_nr];
		            break;
            case NB_F1:
                nb = nb_f1[dev_nr];
		            break;
            case NB_F2:
                nb = nb_f2[dev_nr];
		            break;
            case NB_F3:
                nb = nb_f3[dev_nr];
		            break;
            case NB_F4:
                nb = nb_f4[dev_nr];
		            break;
            case NB_F5:
                nb = nb_f5[dev_nr];
		            break;
            case SB_PCU0:
                nb = sb_pcu0[dev_nr];
		            break;
            case SB_PCU1:
                nb = sb_pcu1[dev_nr];
		            break;
            case SB_PCU2:
                nb = sb_pcu2[dev_nr];
		            break;
            case HSW_PCU0:
                nb = hsw_pcu0[dev_nr];
                break;
            case HSW_PCU1:
                nb = hsw_pcu1[dev_nr];
                break;
            case HSW_PCU2:
                nb = hsw_pcu2[dev_nr];
                break;
            case UNCORE:
                break;
        }
        if (nb) {
            pci_read_config_dword(nb,knob.register_index,&l);
            register_reading = l;
	}
    } else {
        printk(KERN_ERR "Failed to read setting of knob %s. Can not find device %d.\n", knob.name ,dev_nr);
    }
    return register_reading;
}

/* writes the setting to msr / pci knob */
static int write_setting(int dev_nr, struct knob_entry knob, u64 setting) 
{
    u32 l = 0,h = 0,i;
    u64 orig_setting;


    /* check whether setting is valid */
    for (i = 0;i<knob.restricted_settings_length;i++)
        if (setting == knob.restricted_settings[i])
          break;
    /* not one of the restricted settings -> return EINVAL */
    /* if there are restrictions and we didn't match the restriction */
    if (knob.restricted_settings_length && ( i == knob.restricted_settings_length ) )
      return -EINVAL;

    for (i = 0;i<knob.reserved_settings_length;i++)
        if (setting == knob.reserved_settings[i])
          return -EINVAL;


    /* read register */
    orig_setting = read_setting(dev_nr,knob);

    /* avoid a bitmask of 0 and prepare align setting according to bitmask */
    for (i = 0;i<64;i++) {
        if (knob.bitmask&(1ULL<<i))
            break;
    }
    if (i == 64)
        return -EPERM;
    /* align the setting according to the bitmask */
    setting = setting<<i;
    orig_setting &= (~knob.bitmask);
    orig_setting |= setting;
    /* split it in high and low (for MSRs) */
    l = orig_setting&(0xFFFFFFFF);
    h = orig_setting>>32;
    if (dev_nr >= 0) {
        struct pci_dev * nb = NULL;
        switch (knob.device) {
            /* write to msr */
            case MSR:
                if (cpumask_equal(get_cpu_mask(dev_nr),&(current->cpus_allowed))) {
                    native_write_msr(knob.register_index,l,h);
                }
                else {
                    wrmsr_on_cpu(dev_nr, knob.register_index,l, h);
                }
                break;
            case NB_F0:
                nb = nb_f0[dev_nr];
                break;
            case NB_F1:
                nb = nb_f1[dev_nr];
                break;
            case NB_F2:
                nb = nb_f2[dev_nr];
                break;
            case NB_F3:
                nb = nb_f3[dev_nr];
                break;
            case NB_F4:
                nb = nb_f4[dev_nr];
                break;
            case NB_F5:
                nb = nb_f5[dev_nr];
                break;
            case SB_PCU0:
                nb = sb_pcu0[dev_nr];
                break;
            case SB_PCU1:
                nb = sb_pcu1[dev_nr];
                break;
            case SB_PCU2:
                nb = sb_pcu2[dev_nr];
                break;
            case HSW_PCU0:
                nb = hsw_pcu0[dev_nr];
                break;
            case HSW_PCU1:
                nb = hsw_pcu1[dev_nr];
                break;
            case HSW_PCU2:
                nb = hsw_pcu2[dev_nr];
                break;
            case UNCORE:
                break;
        }
        /* write to PCI device */
        if (nb)
           pci_write_config_dword(nb,knob.register_index,l);
        return 0;
    } else {
        printk(KERN_ERR "Failed to write setting of knob %s. Can not find device %d.\n", knob.name ,dev_nr);
        return -ENXIO;
    }
}

/* resolves the device from file and writes the buffer to the knob, that corresponds to ppos */
static ssize_t do_write(struct file * file, const char * buf, 
        size_t count, loff_t *ppos, int type,
        struct knob_entry * entries, u32 entries_length)
{        /*
          * If file position is non-zero, then assume the string has
          * been read and indicate there is no more data to be read.
          */
    struct dentry * de = ((file->f_path).dentry);
    /* get device nr */
    int dev_nr = 0;
    char* end;
    dev_nr = simple_strtol(de->d_name.name,&end,10);
    if (*end) {
        /* special handling, change all settings */
        if (!strcmp(de->d_name.name,"all"))
            dev_nr = -1;
        else
            return -ENXIO;
    } 
    

    if ((*ppos < entries_length)) {
        u64 setting = 0;
        if (count != 8)
            return -EINVAL;
        if (entries[*ppos].readonly)
            return -EPERM;
        setting = ((uint64_t*)buf)[0];
        if (dev_nr == -1) {
            /* write to all devices */
            if (type == X86_ADAPT_CPU) {
            	int i;
                for_each_online_cpu(i) {
                    int ret = write_setting(i,entries[*ppos],setting);
                    if(ret)
                        return ret;
                }
            } else {
            	int i;
                for_each_online_node(i) {
                    int ret = write_setting(i,entries[*ppos],setting);
                    if(ret)
                        return ret;
                }
            }
            return count;
        } else {
            int ret = write_setting(dev_nr,entries[*ppos],setting);
            if (ret) return ret;
            return count;
        }
    } else {
        return -ENXIO;
    }
}

static ssize_t cpu_write(struct file * file, const char * buf, 
        size_t count, loff_t *ppos)
{
    char kernel_buffer[8];
    if (count != 8)
        return -ENXIO;
    if (copy_from_user(kernel_buffer,buf,8)!=0)
        return -ENXIO;

    return do_write(file,kernel_buffer,count,ppos,X86_ADAPT_CPU,active_knobs_cpu,active_knobs_cpu_length);
}

static ssize_t node_write(struct file * file, const char * buf, 
        size_t count, loff_t *ppos)
{
    char kernel_buffer[8];
    if (count != 8)
        return -ENXIO;
    if (copy_from_user(kernel_buffer,buf,8)!=0)
        return -ENXIO;

    return do_write(file,buf,count,ppos,X86_ADAPT_NODE,active_knobs_node,active_knobs_node_length);
}

/* resolves device from file and reads the knob, that corresponds to ppos into the buffer */
static ssize_t do_read(struct file * file, char * buf, 
        size_t count, loff_t *ppos, int type,
        struct knob_entry * entries, u32 entries_length)
{

    /*
     * If file position is non-zero, then assume the string has
     * been read and indicate there is no more data to be read.
     */
    struct dentry * de = ((file->f_path).dentry);
    /* get device nr */
    int dev_nr = 0;
    char* end;
    dev_nr = simple_strtol(de->d_name.name,&end,10);
	if (*end) {
        if (strcmp(de->d_name.name,"all"))
            /* special handling, change all settings */
            return -ENXIO;
        else
            dev_nr = -1;
    } 

    /* print value for id */
    if (*ppos<entries_length) {

        /* setting read from nb / msr */
        u64 setting = 0,tmp = 0;

        /* not enough buffer */
        if (count != 8) {
            // printk("Not enough buffer %zu %u\n",count,8);
            return -EFAULT;
        }

        /* read it */
        if (dev_nr == -1) {
            /* read all devices */
            if (type == X86_ADAPT_CPU) {
            	int i;
                for_each_online_cpu(i) {
                    tmp = read_setting(i,entries[*ppos]);
                    setting |= tmp;
                }
            } else {
            	int i;
                for_each_online_node(i) {
                    tmp = read_setting(i,entries[*ppos]);
                    setting |= tmp;
                }
            }
        } else {
            setting = read_setting(dev_nr,entries[*ppos]);
        }
        /* shift it */
        setting = get_setting_from_register_reading(setting, entries[*ppos].bitmask);
        /* to user buffer */
        if (copy_to_user(buf, &setting, 8)) {
            return -EFAULT;
        }
        //((u64 *)buf)[0] = setting;
        return 8;
    } else {
        // printk(KERN_ERR "Wrong id\n");
        return -ENXIO;
    }
}

static ssize_t cpu_read(struct file * file, char * buf, 
        size_t count, loff_t *ppos)
{
    return do_read(file,buf,count,ppos,X86_ADAPT_CPU,active_knobs_cpu,active_knobs_cpu_length);
}

static ssize_t node_read(struct file * file, char * buf, 
        size_t count, loff_t *ppos)
{
    return do_read(file,buf,count,ppos,X86_ADAPT_NODE,active_knobs_node,active_knobs_node_length);
}

/* writes information about all cpu/node knobs to the buffer */
static ssize_t get_entry_defs(char * buf, size_t count, loff_t *ppos,
        struct knob_entry * defs, u32 length)
{
    if (*ppos == 0) {
        /* write explanation about all available knobs to buffer */
        /* but first, we write 4 byte of total length */
        int position = 0;
        int len,i;
        /* This is the length of the definition data in byte
         * The first 4 byte contain the overall length of the definitions */
        u32 total_length = 4;
        /* we need at least 4 byte to tell the user how long the information data is */
        if (count<4) {
            return -ENOMEM;
        }
        /* the total length needed to store all the knob infos is the most information
         * thus, it has to be written before everything else.
         * Even if the read of this file is only 4 bytes long, */
        for (i = 0;i<length;i++) {
            /* For every knob, we need the header, the name and the description */
            total_length += ENTRY_HEADER_SIZE;
            len = strlen(defs[i].name);
            total_length += len;
            len = strlen(defs[i].description);
            total_length += len;
        }

        /* write total theoretical string length to buf*/
        if (copy_to_user(buf,&total_length,4))
          return -EFAULT;

        /* position in the userspace buffer */
        position = 4;
        /* write all the definitions */
        for (i = 0;i<length;i++) {
            /* if the current item doesn't fit in the userspace buffer, break. */
            if ((position+ENTRY_HEADER_SIZE+strlen(defs[i].name)+
                strlen(defs[i].description))>count)
                break;
            if (copy_to_user(&buf[position],&i,4))
                return -EFAULT;
            position += 4;
            if (copy_to_user(&buf[position],&defs[i].length,1))
              return -EFAULT;
            position += 1;
            len = strlen(defs[i].name);
            if (copy_to_user(&buf[position],&len,4))
              return -EFAULT;
            position += 4;
            if (copy_to_user(&buf[position],defs[i].name,len))
              return -EFAULT;
            position += len;
            len = strlen(defs[i].description);
            if (copy_to_user(&buf[position],&len,4))
              return -EFAULT;
            position += 4;
            if (copy_to_user(&buf[position],defs[i].description,len))
              return -EFAULT;
            position += len;
        }
        *ppos = position;
        return position;
    } else {
        return 0;
    }
}

static ssize_t def_node_read(struct file * file, char * buf, size_t count, loff_t *ppos)
{
    return get_entry_defs(buf,count,ppos,active_knobs_node,active_knobs_node_length);
}

static ssize_t def_cpu_read(struct file * file, char * buf, size_t count, loff_t *ppos)
{
    return get_entry_defs(buf,count,ppos,active_knobs_cpu,active_knobs_cpu_length);
}

/* Called on every open of the cdev (not for every device file!)
 * The are the rules:
 * 1) Multiple readers are ok
 * 2) If someone wants to read, there should be not writers!
 * 3) If someone wants to write, there should be no writers or readers!
 */
static int x86_adapt_open(struct inode * inode, struct file * filep)
{
    int ret = 0;
    struct x86_adapt_cdev *xdev = container_of(inode->i_cdev, struct x86_adapt_cdev, cdev);
    
    if ((filep->f_flags & O_ACCMODE) == O_RDONLY)
    {
        if (!down_read_trylock(&(xdev->rwsem)))
        {
            ret = -EBUSY;
        }
    } 
    else if (!down_write_trylock(&(xdev->rwsem)))
    {
        ret = -EBUSY;
    }
    return ret;
}

static int x86_adapt_release(struct inode *inode, struct file *filp)
{
    struct x86_adapt_cdev *xdev = container_of(inode->i_cdev, struct x86_adapt_cdev, cdev);
    if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
    {
        up_read(&(xdev->rwsem));
    } else {
        up_write(&(xdev->rwsem));
    }
    return 0;
}

static const struct file_operations x86_adapt_cpu_fops = {
    .owner                = THIS_MODULE,
    .llseek               = x86_adapt_seek,
    .open                 = x86_adapt_open,
    .read                 = cpu_read,
    .write                = cpu_write,
    .release              = x86_adapt_release,
};

static const struct file_operations x86_adapt_node_fops = {
    .owner                = THIS_MODULE,
    .llseek               = x86_adapt_seek,
    .open                 = x86_adapt_open,
    .read                 = node_read,
    .write                = node_write,
    .release              = x86_adapt_release,
};

static const struct file_operations x86_adapt_def_cpu_fops = {
    .owner                = THIS_MODULE,
    .read                 = def_cpu_read,
};

static const struct file_operations x86_adapt_def_node_fops = {
    .owner                = THIS_MODULE,
    .read                 = def_node_read,

};

static int x86_adapt_device_create(int cpu)
{
	struct device *dev;

    dev = device_create(x86_adapt_class,NULL,MKDEV(MAJOR(x86_adapt_cpu_device), 
                MINOR(x86_adapt_cpu_device)+cpu),NULL,"cpu%d",cpu);

	return IS_ERR(dev) ? PTR_ERR(dev) : 0;
}

/* function to manage cpu hotplug events */
static int x86_adapt_cpu_callback(struct notifier_block *nfb,
                                unsigned long action, void *hcpu)
{
    unsigned int cpu = (unsigned long) hcpu;
    int err = 0;

    switch (action) {
        case CPU_ONLINE:
        case CPU_ONLINE_FROZEN:
            err = x86_adapt_device_create(cpu);
            break;
        case CPU_DEAD:
        case CPU_DEAD_FROZEN:
            device_destroy(x86_adapt_class,MKDEV(MAJOR(x86_adapt_cpu_device),
                MINOR(x86_adapt_cpu_device)+cpu));
            break;
    }
    return notifier_from_errno(err);
}

static struct notifier_block x86_adapt_cpu_notifier __refdata = {
    .notifier_call = x86_adapt_cpu_callback,
};

/* this function is used to generate the path in /dev 
 * for the corresponding /sys/class/x86_adapt entries
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0)
static char* x86_adapt_devnode(struct device *dev, umode_t *mode)
#else
static char* x86_adapt_devnode(struct device *dev, mode_t *mode)
#endif
{
    int min = MINOR(dev->devt);
    int max = MAJOR(dev->devt);

    if (max == MAJOR(x86_adapt_cpu_device)) {
      if(mode)
          *mode = S_IRUGO | S_IWUSR | S_IWGRP;
        if (min == num_possible_cpus())
            return kasprintf(GFP_KERNEL, "x86_adapt/cpu/all");
        else 
            return kasprintf(GFP_KERNEL, "x86_adapt/cpu/%u", min);
    } else if (max == MAJOR(x86_adapt_node_device)) {
      if(mode)
          *mode = S_IRUGO | S_IWUGO;
        if (min == num_possible_nodes())
            return kasprintf(GFP_KERNEL, "x86_adapt/node/all");
        else
            return kasprintf(GFP_KERNEL, "x86_adapt/node/%u", min);
    } else if (max == MAJOR(x86_adapt_def_cpu_device)) {
      if(mode)
          *mode = S_IRUGO;
        return kasprintf(GFP_KERNEL, "x86_adapt/cpu/definitions");
    } else { /* MAJOR(x86_adapt_def_node_device) */
      if(mode)
          *mode = S_IRUGO;
        return kasprintf(GFP_KERNEL, "x86_adapt/node/definitions");
    }
}


static int __init x86_adapt_init(void)
{
    int i,err;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
    cpu_notifier_register_begin();
#endif
    get_online_cpus();
    if ((x86_adapt_class = class_create(THIS_MODULE, "x86_adapt")) == NULL) {
        printk(KERN_ERR "Failed to create sysfs class\n");
        err = -1;
        goto fail;
    }
    x86_adapt_class->devnode = x86_adapt_devnode;
    ALLOC_AND_INIT(x86_adapt_cpu, DEVICE_CREATE, cpu);
    ALLOC_AND_INIT(x86_adapt_node, DEVICE_CREATE, node);
    ALLOC_AND_INIT(x86_adapt_def_cpu, DEF_CREATE, cpu);
    ALLOC_AND_INIT(x86_adapt_def_node, DEF_CREATE, node);
    err = buildup_entries();
    if (err) {
        printk(KERN_ERR "Failed to allocate memory for cpu/node knobs\n");
        goto fail;
    }
    /* Based on BKDG */
    ALLOC_NB_PCI(0);
    ALLOC_NB_PCI(1);
    ALLOC_NB_PCI(2);
    ALLOC_NB_PCI(3);
    ALLOC_NB_PCI(4);
    ALLOC_NB_PCI(5);

    /* Based on:
     * Intel Xeon Processor E5 Product Family Datasheet- Volume Two: Registers
     * Intel Xeon Processor E5 v2 Product Family Datasheet- Volume Two: Registers */
    ALLOC_UNCORE_PCI(sb_pcu0,10,0);
    ALLOC_UNCORE_PCI(sb_pcu1,10,1);
    ALLOC_UNCORE_PCI(sb_pcu2,10,2);
    ALLOC_UNCORE_PCI(hsw_pcu0,30,0);
    ALLOC_UNCORE_PCI(hsw_pcu1,30,1);
    ALLOC_UNCORE_PCI(hsw_pcu2,30,2);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
    __register_hotcpu_notifier(&x86_adapt_cpu_notifier);
    put_online_cpus();
    cpu_notifier_register_done();
#else
    register_hotcpu_notifier(&x86_adapt_cpu_notifier);
    put_online_cpus();
#endif
    printk(KERN_INFO "Succesfully Started x86 Adapt Processor Feature Device Driver\n");
    return 0;

fail:
    UNREGISTER_AND_DELETE(x86_adapt_cpu, DEVICE_DESTROY, cpu);
    UNREGISTER_AND_DELETE(x86_adapt_node, DEVICE_DESTROY, node);
    UNREGISTER_AND_DELETE(x86_adapt_def_cpu, DEF_DESTROY, cpu);
    UNREGISTER_AND_DELETE(x86_adapt_def_node, DEF_DESTROY, node);
    if (active_knobs_cpu) {
        kfree(active_knobs_cpu);
	active_knobs_cpu = NULL;
    }
    if (active_knobs_node) {
        kfree(active_knobs_node);
	active_knobs_node = NULL;
    }
    FREE_NB(0);
    FREE_NB(1);
    FREE_NB(2);
    FREE_NB(3);
    FREE_NB(4);
    FREE_NB(5);
    FREE_PCI(sb_pcu0);
    FREE_PCI(sb_pcu1);
    FREE_PCI(sb_pcu2);
    FREE_PCI(hsw_pcu0);
    FREE_PCI(hsw_pcu1);
    FREE_PCI(hsw_pcu2);

    put_online_cpus();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
    cpu_notifier_register_done();
#endif
    return err;
}

static void __exit x86_adapt_exit(void)
{
    int i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
    cpu_notifier_register_begin();
#endif
    get_online_cpus();
    UNREGISTER_AND_DELETE(x86_adapt_cpu, DEVICE_DESTROY, cpu);
    UNREGISTER_AND_DELETE(x86_adapt_node, DEVICE_DESTROY, node);
    UNREGISTER_AND_DELETE(x86_adapt_def_cpu, DEF_DESTROY, cpu);
    UNREGISTER_AND_DELETE(x86_adapt_def_node, DEF_DESTROY, node);
    if (active_knobs_cpu) {
        kfree(active_knobs_cpu);
	active_knobs_cpu = NULL;
    }
    if (active_knobs_node) {
        kfree(active_knobs_node);
	active_knobs_node = NULL;
    }
    FREE_NB(0);
    FREE_NB(1);
    FREE_NB(2);
    FREE_NB(3);
    FREE_NB(4);
    FREE_NB(5);
    FREE_PCI(sb_pcu0);
    FREE_PCI(sb_pcu1);
    FREE_PCI(sb_pcu2);
    FREE_PCI(hsw_pcu0);
    FREE_PCI(hsw_pcu1);
    FREE_PCI(hsw_pcu2);

    class_destroy(x86_adapt_class);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
    __unregister_hotcpu_notifier(&x86_adapt_cpu_notifier);
    put_online_cpus();
    cpu_notifier_register_done();
#else
    unregister_hotcpu_notifier(&x86_adapt_cpu_notifier);
    put_online_cpus();
#endif
    printk(KERN_INFO "Shutting Down x86 Adapt Processor Feature Device Driver\n");
}

module_init(x86_adapt_init);
module_exit(x86_adapt_exit);

MODULE_AUTHOR("Robert Schoene <robert.schoene@tu-dresden.de>");
MODULE_AUTHOR("Michael Werner <michael.werner3@tu-dresden.de>");
MODULE_AUTHOR("Joseph Schuchart <joseph.schuchart@tu-dresden.de>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("x86 Adapt Processor Feature Device Driver");
MODULE_VERSION("0.3");
