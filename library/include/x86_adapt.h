/*************************************************************/
/**
* @file x86_adapt.h
* @brief Header File for libx86_adapt
* @author Robert Schoene robert.schoene@tu-dresden.de
*
* x86_adapt
*
* Modulkürzel : x86a
* @version 0.1
*************************************************************/

#ifndef X86_ADAPT_LIB_H_
#define X86_ADAPT_LIB_H_

#include <stdint.h>

/**
 * @typedef an enum for defining the type of device that is requested
 * This can be a CPU (a hardware thread or core) or a die (resp. NUMA node)
 */
typedef enum
{
  X86_ADAPT_CPU=0,
  X86_ADAPT_DIE=1,
  /* Non-Abi */
  X86_ADAPT_MAX=2
} x86_adapt_device_type;

/* represents a single item that can be read or even be written */
struct x86_adapt_configuration_item {
  /* a unique name for an item that can be read or written*/
  char * name;
  /* a description of the item */
  char * description;
  /* the length of the item in bit (max. 64) */
  int length;
};

/** @brief This initializes the library and allocates data structures
 *
 * @return 0 or ErrorCode
 * ErrorCode is:<br>
 * -EIO if files could not be read
 *                   -ENOMEM if data structures could not be allocated
 */
int x86_adapt_init(void);

/** @brief get the number of available devices for device_type
 *
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @return the number of available CPUs, resp. nodes or ErrorCode<br>
 * ErrorCode depends on a call to open() 
 * @see file.h open
 */
int x86_adapt_get_nr_avaible_devices(x86_adapt_device_type device_type);

/** @brief get a read-only file descriptor for a specific CPU or node
 *
 * The file descriptor can later be used to read values from this device.
 * Multiple calls with the same parameter will return the same file descriptor.
 * Each file descriptor has to be unregistered in the library using
 * x86_adapt_put_device().
 * If the device has previously opened using x86_adapt_get_device, the file
 * descriptor will not be updated internally and also provide write access.
 * A device may be opened from several programs in parallel using this function
 * x86_adapt_get_device_ro() or from one program using x86_adapt_get_device()
 * @code
 * if (x86_adapt_init())
 * {
 *    printf("init failed\n");
 *    exit(1);
 * }
 * // get device for CPU 0
 * fd = x86_adapt_get_device_ro(X86_ADAPT_CPU,0);
 * if (fd < 0)
 * {
 *    printf("open failed\n");
 *    exit(1);
 * }
 * // ... (read some values from CPU 0)
 * @endcode
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @param nr the index of the CPU or node for which you need the fd
 * @return a file descriptor to use later or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid<br>
 *               other depending on whether the file
 *               /dev/x86_adapt/[cpu|node]/<nr> could be opened
 * @see file.h open, x86_adapt_get_setting, x86_adapt_get_device_ro, x86_adapt_put_device
 */
int x86_adapt_get_device_ro(x86_adapt_device_type device_type, uint32_t nr);

/** @brief get a file descriptor for a specific CPU or node
 *
 * The file descriptor can later be used to read or write values from
 * or to this device
 * Multiple calls with the same parameter will return the same file descriptor.
 * Each file descriptor has to be unregistered in the library using
 *  x86_adapt_put_device.
 * If the device has previously opened using x86_adapt_get_device_ro, the file
 * descriptor will not be updated only provide read-only access.
 * A device may be opened from several programs in parallel using
 * x86_adapt_get_device_ro() or from one program using x86_adapt_get_device()
 * @code
 * if (x86_adapt_init())
 * {
 *    printf("init failed\n");
 *    exit(1);
 * }
 * // get device for CPU 0
 * fd = x86_adapt_get_device(X86_ADAPT_CPU,0);
 * if (fd < 0)
 * {
 *    printf("open failed\n");
 *    exit(1);
 * }
 * // ... (read or write some values from or to CPU 0)
 * @endcode
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @param nr the index of the CPU or node for which you need the fd
 * @return a file descriptor to use later or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid<br>
 *               other depending on whether the file
 *               /dev/x86_adapt/[cpu|node]/<nr> could be opened
 * @see file.h open, x86_adapt_get_setting, x86_adapt_set_setting, x86_adapt_get_device, x86_adapt_put_device
 */
int x86_adapt_get_device(x86_adapt_device_type device_type, uint32_t nr);

/** @brief put a file descriptor for a specific CPU or node
 *
 * deregisters a file descriptor in the library. This function should be called
 * as often as the file descriptor is get via @see x86_adapt_get_device_ro or
 * @see x86_adapt_get_device
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @param nr the index of the CPU or node for which you need the fd
 * @return 0 or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid
 */
int x86_adapt_put_device(x86_adapt_device_type device_type, uint32_t nr);

/** @brief get a file descriptor for all CPUs or nodes
 *
 * The file descriptor can later be used to read values from these devices.
 * The all_devices file descriptor is of limited use for reading as it returns
 * the bitwise or'ed values from all instances of CPUs or nodes.
 * Multiple calls with the same parameter will return the same file descriptor.
 * Each file descriptor has to be unregistered in the library using
 * x86_adapt_put_all_devices().
 * If the device has previously opened using x86_adapt_get_all_devices(), the
 * file descriptor will not be updated internally and also provide write access.
 * A device may be opened from several programs in parallel using
 * x86_adapt_get_all_devices_ro() or from one program using
 * x86_adapt_get_all_devices()
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @return a file descriptor to use later or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid<br>
 *               other depending on whether the file
 *               /dev/x86_adapt/[cpu|node]/<nr> could be opened
 * @see file.h open, x86_adapt_get_setting, x86_adapt_get_device_ro, 
 *      x86_adapt_put_all_devices
 */
int x86_adapt_get_all_devices_ro(x86_adapt_device_type device_type);

/** @brief get a read-write file descriptor for all CPUs or nodes
 *
 * The file descriptor can later be used to read values from
 * these devices via x86_adapt_get_setting()
 * The file descriptor can later be used to write values from
 * these devices via x86_adapt_set_setting()
 * Multiple calls with the same parameter will return the same file descriptor.
 * Each file descriptor has to be unregistered in the library using
 * x86_adapt_put_all_devices().
 * If the device has previously opened using x86_adapt_get_all_devices(), the
 * file descriptor will not be updated internally and also provide write access.
 * A device may be opened from several programs in parallel using
 * x86_adapt_get_all_devices_ro()
 * or from one program using x86_adapt_get_all_devices()
 * The all_devices file descriptor is of limited use for reading as it returns
 * the bitwise or'ed values from all instances of CPUs or nodes. However, it can
 * be useful for writing.
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @return a file descriptor to use later or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid<br>
 *               other depending on whether the file
 *               /dev/x86_adapt/[cpu|node]/<nr> could be opened
 * @see file.h open, x86_adapt_get_setting, x86_adapt_set_setting,
 * x86_adapt_get_all_devices_ro, x86_adapt_put_all_devices
 */
int x86_adapt_get_all_devices(x86_adapt_device_type device_type);

/** 
 * @brief put a file descriptor for a specific CPU or node
 *
 * deregisters a file descriptor in the library. This function should be called
 * as often as the file descriptor is get via x86_adapt_get_all_devices_ro()
 * or x86_adapt_get_all_devices()
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @param nr the index of the CPU or node for which you need the fd
 * @return 0 or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid
 */
int x86_adapt_put_all_devices(x86_adapt_device_type device_type);

/**
 * @brief retrieve the description of a configuration item from a device
 *
 * Returns the name, description and length in bit for a specific configuration
 * item.
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @param id the ID of the configuration item. The IDs are numbered sequentially
 *           starting with 0. The number of configuration items for a device
 *           type can be retrieved using @see x86_adapt_get_number_cis()
 * @param item. A pointer to a valid struct x86_adapt_configuration_item instance.
 *              This parameter is changed from the function!
 *              The returned strings in item.description and item.name shall not
 *              be free()d!
 * @return 0 or ErrorCode<br>
 * ErrorCode is: -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type or nr is invalid
 * @see file.h open, x86_adapt_get_setting, x86_adapt_set_setting,
 * x86_adapt_get_all_devices_ro, x86_adapt_get_all_devices
 */
int x86_adapt_get_ci_definition(x86_adapt_device_type device_type, uint32_t id,struct x86_adapt_configuration_item * item);

/**
 * @brief retrieve the number of available configuration item for a device type
 * @code
 * // initialize
 * if (x86_adapt_init())
 * {
 *    printf("init failed\n");
 *    exit(1);
 * }
 * // get number of CPU definitions
 * ret = x86_adapt_get_number_cis(X86_ADAPT_CPU);
 * if (ret < 0)
 * {
 *    printf("could not read number of CPU settings\n");
 *    exit(1);
 * }
 * // read and print all item definitions
 * for (id=0;id<ret;id++)
 * {
 *    struct x86_adapt_configuration_item item;
 *    if (x86_adapt_get_ci_definition(X86_ADAPT_CPU,id,&item))
 *    {
 *      printf("could not read CPU definition %d\n",id);
 *      exit(1);
 *    }
 *    else 
 *    {
 *      printf("CPU definition %d: %s (%s)\n",id,item->name, item->description);
 *    }
 * }
 * @endcode
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @return 0 or ErrorCode<br>
 * ErrorCode is: -EPERM if the library is not initialized yet<br>
 *               -ENXIO if device_type is invalid
 */
int x86_adapt_get_number_cis(x86_adapt_device_type device_type);

/**
 * @brief retrieve the configuration item ID for a specific name and device type
 * @code
 * // initialize
 * if (x86_adapt_init())
 * {
 *    printf("init failed\n");
 *    exit(1);
 * }
 * // look up CPU definition I know in beforehand
 * id = x86_adapt_lookup_ci_name(X86_ADAPT_CPU,"Intel_CORE_C3_RESIDENCY");
 * if (id < 0)
 * {
 *    printf("could not find Intel_CORE_C3_RESIDENCY\n");
 *    exit(1);
 * }
 * // ... (read value from some CPU)
 * @endcode
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 * @param name the name of the configuration item
 * @return >=0 as configuration item ID or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 * -ENXIO if device_type is invalid or name could not be found
 */
int x86_adapt_lookup_ci_name(x86_adapt_device_type device_type, const char * name);

/**
 * @brief get the setting of a configuration item
 *
 * @param fd a file descriptor retrieved with x86_adapt_get_device_ro(),
 * x86_adapt_get_device(), x86_adapt_get_all_devices_ro(), or
 * x86_adapt_get_all_devices()
 * @param id the configuration item ID retrieved with x86_adapt_lookup_ci_name()
 * or x86_adapt_get_ci_definition()
 * @param setting a pointer to a uint64_t datastructure where the reading will
 * be stored
 * @return 8 or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 * others depending on the kernel module
 * @see x86_adapt_get_device, x86_adapt_get_device,
 * x86_adapt_get_all_devices_ro, x86_adapt_get_all_devices,
 * x86_adapt_lookup_ci_name, x86_adapt_get_ci_definition
 */
int x86_adapt_get_setting(int fd, int id, uint64_t * setting);

/**
 * @brief change the setting of a configuration item
 *
 * @param fd a file descriptor retrieved with x86_adapt_get_device() or
 * x86_adapt_get_all_devices()
 * @param id the configuration item ID retrieved with x86_adapt_lookup_ci_name()
 * or x86_adapt_get_ci_definition()
 * @param setting the new setting for the configuration item
 * @return 8 or ErrorCode<br>
 * ErrorCode is:<br>
 * -EPERM if the library is not initialized yet<br>
 * others depending on the kernel module (e.g. if you're to write
 *               a read-only value)
 * @see x86_adapt_get_device, x86_adapt_get_device, x86_adapt_lookup_ci_name,
 * x86_adapt_get_ci_definition
 */
int x86_adapt_set_setting(int fd, int id, uint64_t setting);

/**
 * @brief This closes all still-open file descriptors and free's data structures
 */
void x86_adapt_finalize(void);

#endif
