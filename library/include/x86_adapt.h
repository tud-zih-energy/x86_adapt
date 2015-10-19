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
* 
* \defgroup func "Functions"
*************************************************************/

/**
 * @mainpage 
 * @section main X86_ADAPT Package
 * @subsection intro Introduction
 * The x86_adapt library can be used to query control low-level features of the CPU. 
 * It provides safe access to machine specific registers (MSR) through a kernel module abstraction. 
 * Through this abstraction, values can be read and written based on the configuration of the kernel module. 
 * This configuration is provided at module build time through a set of so-called knob files describing the MSR and the possible values that it can take. 
 * Any other value won't be permitted by the kernel module. 
 *
 * @subsection building Build instructions
 * The kernel module is built using CMake. Please see the instructions in the package README.md on how built it. 
 *
 * @subsection kernelModule Accessing the kernel module
 * @subsubsection libAccess Using the x86_adapt library
 * Please see the documentation of the x86_adapt.h header file for details on how to access the MSRs through a library.
 * 
 * @subsubsection directAccess Direct access
 * The kernel module creates a set of device files through which it communicates. 
 * The page @ref directA describes how to use the x86_adapt kernel module directly through the device files. 
 * 
 */

/**
 * @page directA Direct Access

@section folders Available folders

- /dev/x86_adapt/cpu - Holds devices for CPUs (hardware threads)
- /dev/x86_adapt/node - Holds devices for nodes (NUMA nodes)

@subsection files Files within these folders
- ./definition - Definition file (see reading definitions)
- ./all - a virtual device for reading and writing the configuration items of all instances of type cpu or node
- ./<num> - a virtual device for reading and writing the configuration items of a specific instance of type cpu or node

@section rw Reading and Writing

@subsection readingDefs Reading definitions
Read the definitions in two steps

1. do a 4 byte read on offset 0 on file /dev/x86_adapt/[cpu|node]/definition
This will return the number of bytes to read all the difinitions for cpus, resp. nodes (nr_bytes).
This read can be skipped if it si known beforehand how large the second read should be.

2. do a read with nr_bytes or more bytes on offset 0 on file /dev/x86_adapt/[cpu|node]/definition
This will return all the information, including names, descriptions and variable lengths

Read this file like
@code
returned = pread(fd, memory_buffer, nr_bytes, offset);
@endcode
@param fd the file descriptor of a definitions file (in)
@param memory_buffer a memory buffer (out)<br>
 The kernel will write configuration item definitions to this buffer until (a)
 the buffer is full or (b) all configuration items are written.<br>
 See the code section below for an encoding of the memory_buffer
@param nr_bytes the size of the memory buffer (in)
@param offset is ignored, should be 0 (in)
@return the number of bytes written to memory_buffer or ErrorCode<br>
 Errorcode can be:<br>
 - -EFAULT the kernel could not copy information to the memory_buffer
 - -ENOMEM nr_bytes < 4


@code

struct configuration_item_entry
{
  // the ids are enumerated, so the first configuration_item_entry
  // in a struct memory_buffer_encoding will have id 0, the second entry id 1,
  // the n-th entry n-1
  int32_t id;
  uint8_t length_in_bit;
  // length of name field
  int32_t name_length;
  // name of the item (does not contain an ending \0)
  char name[name_length];
  // length of description field
  int32_t description_length;
  // description of the item (does not contain an ending \0)
  char description[description_length];
}

struct memory_buffer_encoding
{
  int32_t total_length_in_byte;
  // if there is any configuration_item
  // check total_length_in_byte to see if there are more entries to come
  struct configuration_item_entry entry_0;
  // if there is a second configuration_item
  struct configuration_item_entry entry_1;
  // if there is a third configuration_item
  struct configuration_item_entry entry_2;
  // ...
}

uint32_t size_read;
size_t bytes;
char * information;
int fd = open("/dev/x86_adapt/cpu/definition",O_RDONLY)
bytes = pread(fd, &size_read, 4, 0);
if (bytes != 4)
{
  printf("Error initial reading /dev/x86_adapt/cpu/definition: %d",size_read);
  exit(1);
}
information=malloc(bytes);
bytes = pread(fd,memory_buffer,size_read,0);
if (bytes != size_read )
{
  printf("Error second reading /dev/x86_adapt/cpu/definition: %d",size_read);
  exit(1);
}
// information holds now struct memory_buffer_encoding
...
@endcode

@subsection readSettings Reading settings
Read an 8 byte value with the setting to /dev/x86_adapt/[cpu|node]/<nr> at a certain offset
via 
@code
returned = pread(fd, memory_buffer, nr_bytes, id);
@endcode
@param fd the file descriptor of a configuration device file (in)
@param memory_buffer a memory buffer which can hold 8 byte (e.g., a pointer to an uint64) (out)<br>
 The kernel will write the reading of the item with the given id to this buffer
 If the fd links to an ./all file, the items of all available
 cpus or nodes will be bitwise ored
@param nr_bytes MUST be 8 (in)
@param id is the id of a configuration item which can be retrieved by reading it from
 /dev/x86_adapt/[cpu|node]/definition (in)
@return the number of bytes written to reading or ErrorCode<br>
 Errorcode can be:<br>
 - -EFAULT the kernel could not copy information to the memory_buffer or nr_bytes is != 8
 - -ENXIO the id is invalid

@code
uint64_t value;
size_t bytes;
int fd = open("/dev/x86_adapt/cpu/0",O_RDONLY);
if (fd < 0)
{
  printf("Error opening /dev/x86_adapt/cpu/0: %d",fd);
  exit(1);
}
bytes = pread(fd, &value, 8, 0);
if (bytes != 8)
{
  printf("Error reading /dev/x86_adapt/cpu/0: %d",size_read);
  exit(1);
}
printf("Configuration item 0 has setting %"PRIu64"\n",value);
@endcode

@subsection writeSettings Writing settings
Write an 8 byte value with the setting to /dev/x86_adapt/[cpu|node]/<nr> at a certain offset via
@code
returned = pread(fd, memory_buffer, nr_bytes, id);
@endcode
@param fd the file descriptor of a configuration device file (in)
@param memory_buffer a memory buffer which can hold 8 byte (e.g., a pointer to an uint64) (out)<br>
 The kernel will write the reading of the the setting in this buffer to the device(s)
 If the fd links to an ./all file, the items of all available
 cpus or nodes will be written
@param nr_bytes MUST be 8 (in)
@param id is the id of a configuration item which can be retrieved by reading it from
 /dev/x86_adapt/[cpu|node]/definition (in)
@return the number of bytes written to reading or ErrorCode<br>
 Errorcode can be:
 - -EFAULT the kernel could not copy information from the memory_buffer or nr_bytes is != 8
 - -ENXIO the id is invalid, the device could not be found internally
 - -EINVAL if the setting is restricted / reserved
 - -EPERM if it is a read-only setting

@code
uint64_t value=0;
size_t bytes;
int fd = open("/dev/x86_adapt/cpu/0",O_RDWR);
if (fd < 0)
{
  printf("Error opening /dev/x86_adapt/cpu/0: %d",fd);
  exit(1);
}
bytes = pwrite(fd, &value, 8, 0);
if (bytes != 8)
{
  printf("Error writing /dev/x86_adapt/cpu/0: %d",size_read);
  exit(1);
}
printf("Set configuration item 0 of CPU 0 to %"PRIu64"\n",value);
@endcode
 */

#ifndef X86_ADAPT_LIB_H_
#define X86_ADAPT_LIB_H_

#include <stdint.h>

/**
 * @typedef  x86_adapt_device_type
 * \brief an enum for defining the type of device that is requested <br>
 * This can be a CPU (a hardware thread or core) or a die (resp. NUMA node)
 */
typedef enum
{
  X86_ADAPT_CPU=0,
  X86_ADAPT_DIE=1,
  /* Non-Abi */
  X86_ADAPT_MAX=2
} x86_adapt_device_type;

/**
 * \struct x86_adapt_configuration_item
 * \brief represents a single item that can be read or even be written 
 */
struct x86_adapt_configuration_item {
  char * name;        /**< a unique name for an item that can be read or written */
  char * description; /**< a description of the item */ 
  int length;         /**< the length of the item in bit (max. 64) */
};

/*!
 * @brief This initializes the library and allocates internal data structures
 * @return 0 or ErrorCode
 * ErrorCode is:<br>
 *   - -EIO if files could not be read <br>
 *   - -ENOMEM if data structures could not be allocated
 */
int x86_adapt_init(void);

/** @brief get the number of available devices for device_type
 *
 * @param device_type can be X86_ADAPT_CPU or X86_ADAPT_DIE from the enum x86_adapt_device_type
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
 * - -EPERM if the library is not initialized yet<br>
 * - -ENXIO if device_type or nr is invalid<br>
 * - other depending on whether the file
 *   /dev/x86_adapt/[cpu|node]/<nr> could be opened
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
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type or nr is invalid
 * - other depending on whether the file
 *   /dev/x86_adapt/[cpu|node]/<nr> could be opened
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
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type or nr is invalid
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
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type or nr is invalid
 * - other depending on whether the file
 *   /dev/x86_adapt/[cpu|node]/<nr> could be opened
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
 * ErrorCode is:
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type or nr is invalid
 * - other depending on whether the file
 * /dev/x86_adapt/[cpu|node]/<nr> could be opened
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
 * @return 0 or ErrorCode<br>
 * ErrorCode is:
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type or nr is invalid
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
 * @param item A pointer to a valid struct x86_adapt_configuration_item instance.
 *              This parameter is changed from the function!
 *              The returned strings in item.description and item.name shall not
 *              be free()d!
 * @return 0 or ErrorCode<br>
 * ErrorCode is: 
 * - -EPERM if the library is not initialized yet
 * - ENXIO if device_type or nr is invalid
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
 * ErrorCode is: 
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type is invalid
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
 * ErrorCode is:
 * - -EPERM if the library is not initialized yet
 * - -ENXIO if device_type is invalid or name could not be found
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
 * ErrorCode is:
 * - -EPERM if the library is not initialized yet
 * - others depending on the kernel module
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
 * ErrorCode is:
 * - -EPERM if the library is not initialized yet
 * - others depending on the kernel module (e.g. if you're to write a read-only value)
 * @see x86_adapt_get_device, x86_adapt_get_device, x86_adapt_lookup_ci_name,
 * x86_adapt_get_ci_definition
 */
int x86_adapt_set_setting(int fd, int id, uint64_t setting);

/**
 * @brief This closes all still-open file descriptors and free's data structures
 */
void x86_adapt_finalize(void);

#endif
