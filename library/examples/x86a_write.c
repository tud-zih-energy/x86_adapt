/*************************************/
/**
 * @file x86a_write.c
 * @brief Example application to set the value of a x86_adapt knob on the system. 
 * 
 * Please see the help text (-h) for details on how to use it. 
 * The most common way is to specify the option or knob via -i and the value to be set via -V:
 * @code 
 * ./x86a_write -i Intel_Package_CState_Limit -V 3
 * @endcode
 *
 * Use the tool @ref x86a_read.c "x86a_read" to query all available knobs and their values on the system.
 * 
 * @TODO merge code from x86a_read.c and x86a_write.c
 * @author Robert Schoene robert.schoene@tu-dresden.de 
 *************************************/ 

#include <stdio.h>
#include "x86_adapt.h"
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <libgen.h>

static void print_help(char ** argv)
{
  char *path = strdup(argv[0]);
  char *base = basename(path);
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s [ %s-ARGS ...] -i <item name> -V <value>\n", base, base);
  fprintf(stderr, "\n");
  fprintf(stderr, "%s-ARGS:\n", base);
  fprintf(stderr, "\t -h --help: print this help\n");
  fprintf(stderr, "\t -H --hex: set in hexadecimal form\n");
  fprintf(stderr, "\t -n --node: set node options instead of CPU options\n");
  fprintf(stderr, "\t -c --cpu: Set item of this CPU (default=all)\n"
                        "\t\tIf -n is set, set the item of this node (default=all).\n");
  fprintf(stderr, "\t -i --item: set this item\n");
  fprintf(stderr, "\t -V --value: set to this value\n");
  fprintf(stderr, "\t -v --verbose: print more information\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "This program allows users to write x86_adapt CPU and node knobs.\n");
  free(path);
}

static void print_cis(x86_adapt_device_type type, int verbose)
{
  struct x86_adapt_configuration_item item;
  int nr_items = x86_adapt_get_number_cis(type);
  int ci_nr = 0;
  fprintf(stdout,"Available items:\n");
  for ( ci_nr = 0 ; ci_nr < nr_items ; ci_nr++ )
  {
    if (x86_adapt_get_ci_definition(type,ci_nr,&item))
    {
      fprintf(stderr,"Could not read item definition\n");
      exit(1);
    }
    if (verbose)
      fprintf(stdout,"Item %s\n%s\n----------------\n",item.name,item.description);
    else
      fprintf(stdout,"Item %s\n----------------\n",item.name);
  }
}

static void set_cpu(x86_adapt_device_type type, int cpu, int item, uint64_t new_setting,int verbose)
{
  int fd = x86_adapt_get_device(type, cpu);
  int ret;
  if ( fd > 0 )
  {
    if ((ret = x86_adapt_set_setting(fd,item,new_setting)) != 8)
    {
      fprintf(stderr,"Could not set item for %s %d\n",type==X86_ADAPT_DIE?"node":"CPU",cpu);
      return;
    }
    x86_adapt_put_device(type, cpu);
  } else
  {
    fprintf(stderr,"Could not open /dev/x86_adapt/%s/%d file\n",type==X86_ADAPT_DIE?"node":"CPU",cpu);
    return;
  }
}

int main(int argc, char ** argv)
{
  int cpu=-1;
  int hex=0;
  char c;
  int verbose=0;
  x86_adapt_device_type type = X86_ADAPT_CPU;
  char * knob=NULL;
  char * new_setting_str=NULL;
  int64_t new_setting=-1;
  struct x86_adapt_configuration_item item;
  int nr_items,ci_nr;


  while (1)
  {
    static struct option long_options[] =
      {
          {"help", no_argument, 0, 'h'},
          {"hex", no_argument, 0, 'H'},
          {"item", required_argument, 0, 'i'},
          {"value", required_argument, 0, 'V'},
          {"node", no_argument, 0, 'n'},
          {"cpu", required_argument, 0, 'c'},
          {"verbose", no_argument, 0, 'v'},
          {0,0,0,0}
      };
    int option_index = 0;

    c = getopt_long(argc, argv, "hHc:i:V:nv", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 'h':
      print_help(argv);
      return 1;
    case 'H':
      hex = 1;
      break;
    case 'n':
      type = X86_ADAPT_DIE;
      break;
    case 'c':
      cpu = atoll(optarg);
      break;
    case 'i':
      knob=strdup(optarg);
      break;
    case 'V':
      new_setting_str=strdup(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      break;
    }
  } /* end of parse arguments */

  if (x86_adapt_init())
  {
    fprintf(stderr,"Could not initialize x86_adapt library");
  }

  if (knob == NULL || new_setting_str == NULL)
  {
    fprintf(stderr,"The flags -o and -V are mandatory. Available options are listed now:\n");
    print_cis(type, verbose);
    exit(1);
  }

  /* get item index */
  nr_items = x86_adapt_get_number_cis(type);
  for ( ci_nr = 0 ; ci_nr < nr_items ; ci_nr++ )
  {
    if (x86_adapt_get_ci_definition(type,ci_nr,&item))
    {
      fprintf(stderr,"Could not read item definition\n");
      exit(1);
    }
    if (strcmp(item.name,knob)==0)
    {
      break;
    }
  }
  /* not found? */
  if ( ci_nr==nr_items )
  {
    fprintf(stderr,"The option %s is not available. Available options are listed now:\n",knob);
    print_cis(type, verbose);
    exit(1);
  }

  /* convert setting friom string to int */
  if ( hex )
    new_setting=strtoll(new_setting_str,NULL,16);
  else
    new_setting=strtoll(new_setting_str,NULL,10);

  /* set setting */
  if ( cpu == -1 )
  {
    for ( cpu = 0 ; cpu < x86_adapt_get_nr_available_devices(type) ; cpu++ )
    {
      if  (verbose)
        fprintf(stdout,"Set definition %s on %s %d to %"PRIi64"\n",knob,type==X86_ADAPT_DIE?"node":"CPU",cpu,new_setting);
      set_cpu(type, cpu, ci_nr, new_setting,verbose);
    }
  } else
  {
    if  (verbose)
      fprintf(stdout,"Set definition %s on %s %d to %"PRIi64"\n",knob,type==X86_ADAPT_DIE?"node":"CPU",cpu,new_setting);
    set_cpu(type, cpu, ci_nr, new_setting,verbose);
  }

  return 0;
}
