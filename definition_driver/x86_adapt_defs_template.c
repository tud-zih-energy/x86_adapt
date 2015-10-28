#include <linux/module.h>

#include "../definition_driver/x86_adapt_defs.h"

#define MODULE_NAME "x86_adapt_definition"


/* here should be all the information inserted by python */

#template_holder

/* here should be all the information inserted by python */

struct knob_entry_definition * get_all_knobs(void)
{
  return all_knobs;
}

u32 get_all_knobs_length(void)
{
  return all_knobs_length;
}

EXPORT_SYMBOL(get_all_knobs);
EXPORT_SYMBOL(get_all_knobs_length);

static int __init x86_adapt_definition_init(void)
{
  int err=0;
  printk("Init\n");
  return err;
}

static void __exit x86_adapt_definition_exit(void)
{
  printk("Exit\n");
}

module_init(x86_adapt_definition_init);
module_exit(x86_adapt_definition_exit);
MODULE_AUTHOR("Robert Schoene <robert.schoene@tu-dresden.de>");
MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("x86 Adapt Processor Feature Device Definition Driver");
MODULE_VERSION("0.1");
