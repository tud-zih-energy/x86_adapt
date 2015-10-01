#ifndef X86_ADAPT_LIB_H_
#define X86_ADAPT_LIB_H_

#include <stdint.h>

typedef enum
{
  X86_ADAPT_CPU=0,
  X86_ADAPT_DIE=1,
  /* Non-Abi */
  X86_ADAPT_MAX=2
} x86_adapt_device_type;

struct x86_adapt_configuration_item {
	char * name;
	char * description;
	int length;
};

/* TODO: Add description or return values */

/* This should initialize the library and allocate data structures */
int x86_adapt_init(void);

/* 
 * get the number of available devices for device_type
 * device type can be X86_ADAPT_CPU or X86_ADAPT_DIE
 */
int x86_adapt_get_nr_avaible_devices(x86_adapt_device_type device_type);

/* returns file descriptor for /dev/x86_adapt/<cpu|node>/<nr> in read only mode*/
int x86_adapt_get_device_ro(x86_adapt_device_type device_type, uint32_t nr);

/* returns file descriptor for /dev/x86_adapt/<cpu|node>/<nr> */
int x86_adapt_get_device(x86_adapt_device_type device_type, uint32_t nr);

/* closes file descriptor */
int x86_adapt_put_device(x86_adapt_device_type device_type, uint32_t nr);

/* returns file descriptor for /dev/x86_adapt/all in read only mode*/
int x86_adapt_get_all_devices_ro(x86_adapt_device_type device_type);

/* returns file descriptor for /dev/x86_adapt/all */
int x86_adapt_get_all_devices(x86_adapt_device_type device_type);

/* closes file descriptor */
int x86_adapt_put_all_devices(x86_adapt_device_type device_type);

/* returns 0 and item for device type and id, otherwise error */
int x86_adapt_get_ci_definition(x86_adapt_device_type device_type, uint32_t id,struct x86_adapt_configuration_item * item);

/* returns number of configuration items for device type or negative if error */
int x86_adapt_get_number_cis(x86_adapt_device_type device_type);

/* returns the index number of the supplied configuration item */
int x86_adapt_lookup_ci_name(x86_adapt_device_type device_type, const char * name);

/*  for a specific configuration item */
/*  returns 8 if there has been no error */
int x86_adapt_get_setting(int fd, int id, uint64_t * setting);

/*  for a specific configuration item */
int x86_adapt_set_setting(int fd, int id, uint64_t setting);

/* This should finalize the library, close fd's and free data structures */
void x86_adapt_finalize(void);

#endif
