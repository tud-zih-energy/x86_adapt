#ifndef X86_ADAPT_LIB_H_
#define X86_ADAPT_LIB_H_

#include <stdint.h>

#define X86_ADAPT_CPU 0
#define X86_ADAPT_DIE 1

struct x86_adapt_configuration_item {
	char * name;
	char * description;
	int length;
};

struct x86_adapt_avaible_devices {
    int count;
    int * devices;
};

/* TODO: Add description or return values */

/* 
 * This looks in /sys/devices/system/cpu/online 
 * The user is responsible for freeing the returned memory chunk.
 */
struct x86_adapt_avaible_devices* get_avaible_cpus(void);

/* This looks in /sys/devices/system/node/online 
 * The user is responsible for freeing the returned memory chunk.
 */
struct x86_adapt_avaible_devices* get_avaible_dies(void);

/* This should initialize the library and allocate data structures */
int x86_adapt_init(void);

/* returns file descriptor for /dev/x86_adapt/<cpu|node>/<nr> in read only mode*/
int x86_adapt_get_device_ro(uint32_t device_type, uint32_t nr);

/* returns file descriptor for /dev/x86_adapt/<cpu|node>/<nr> */
int x86_adapt_get_device(uint32_t device_type, uint32_t nr);

/* closes file descriptor */
int x86_adapt_put_device(uint32_t device_type, uint32_t nr);

/* returns file descriptor for /dev/x86_adapt/all in read only mode*/
int x86_adapt_get_all_devices_ro(uint32_t device_type);

/* returns file descriptor for /dev/x86_adapt/all */
int x86_adapt_get_all_devices(uint32_t device_type);

/* closes file descriptor */
int x86_adapt_put_all_devices(uint32_t device_type);

/* returns 0 and item for device type and id, otherwise error */
int x86_adapt_get_ci_definition(uint32_t device_type, uint32_t id,struct x86_adapt_configuration_item ** item);

/* returns number of configuration items for device type or negative if error */
int x86_adapt_get_number_cis(uint32_t device_type);

/* returns the index number of the supplied configuration item */
int x86_adapt_lookup_ci_name(uint32_t device_type, const char * name);

/*  for a specific configuration item */
int x86_adapt_get_setting(int fd, int id, uint64_t * setting);

/*  for a specific configuration item */
int x86_adapt_set_setting(int fd, int id, uint64_t setting);

/* This should finalize the library, close fd's and free data structures */
void x86_adapt_finalize(void);

#endif
