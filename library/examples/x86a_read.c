/*************************************/
/**
 * @file x86a_read
 * @brief Example application to read the values of all available knobs on the system. 
 * 
 * @synopsis x86a_read [ x86a_read-ARGS ...]
 * Invoking the x86a_read tool prints a list of knobs:
 * @code
 * Item 0: Intel_Clock_Modulation_Value
 * ----------------
 * Item 1: Intel_DCU_Prefetch_Disable
 * ----------------
 * Item 2: Intel_Enhanced_SpeedStep
 * ----------------
 * Item 3: Intel_Target_PState
 * [...]
 * @endcode
 * Add \-\-verbose to get a short description of each knob. <br> 
 * The tool also prints a the current settings of each knob as CSV.
 * 
 * Options and Usage:
 * @code
 * Usage: x86a_read [ x86a_read-ARGS ...]
 * x86a_read-ARGS:
 * 	 -h --help: print this help
 * 	 -H --hex: print readings in hexadecimal form
 * 	 -n --node: print node options instead of CPU options
 * 	 -c --cpu: Read item(s) from this CPU (default=all)
 * 		If -n is set, read item(s) from this node (default=all).
 * 	 -i --item: Read this item (default=all)
 * 	 -v --verbose: print more information
 * @endcode
 * Details:
 * The term "node" refers to a node in the Linux context, which is a NUMA node (e.g., a physical processor).
 * Examples:
 * 
 * @code
 * x86a_read -c 0 # prints all items and the readings for CPU 0
 * x86a_read -n 0 -c 0 # prints all items and the readings for node 0
 * x86a_read -n 0 -c 0 -X # prints all items and the readings for node 0 in hexadecimal
 * x86a_read -i Intel_Clock_Modulation_Value -c 0 -X # prints the item Intel_Clock_Modulation_Value and its reading for CPU 0 in hexadecimal
 * @endcode
 *
 * Use the tool @ref x86a_write.c "x86a_write" to change a setting.
 *
 * @TODO merge code from x86a_read.c and x86a_write.c
 * @author Robert Schoene robert.schoene@tu-dresden.de 
 */

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
  fprintf(stderr, "Usage: %s [ %s-ARGS ...]\n", base, base);
  fprintf(stderr, "\n");
  fprintf(stderr, "%s-ARGS:\n", base);
  fprintf(stderr, "\t -h --help: print this help\n");
  fprintf(stderr, "\t -H --hex: print readings in hexadecimal form\n");
  fprintf(stderr, "\t -n --node: print node options instead of CPU options\n");
  fprintf(stderr, "\t -c --cpu: Read item(s) from this CPU (default=all)\n"
                     "\t\tIf -n is set, read item(s) from this node (default=all).\n");
  fprintf(stderr, "\t -i --item: Read this item (default=all)\n");
  fprintf(stderr, "\t -v --verbose: print more information\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "This program reads all available CPU/node knobs from x86_adapt.\n");
  free(path);
}

static void print_cis(x86_adapt_device_type type, int verbose, int item_min, int item_max)
{
  struct x86_adapt_configuration_item item;
  int ci_nr = 0;
  for ( ci_nr = item_min ; ci_nr < item_max ; ci_nr++ )
  {
    if (x86_adapt_get_ci_definition(type,ci_nr,&item))
    {
      fprintf(stderr,"Could not read item definition\n");
      exit(1);
    }
    if (verbose)
      fprintf(stdout,"Item %i: %s\n%s\n----------------\n",ci_nr,item.name,item.description);
    else
      fprintf(stdout,"Item %i: %s\n----------------\n",ci_nr,item.name);
  }
}

static void print_header(x86_adapt_device_type type, int item_min, int item_max)
{
  int ci_nr = 0;

  switch (type)
  {
    case X86_ADAPT_CPU:
      fprintf(stdout,"CPU");
      break;
    case X86_ADAPT_DIE:
      fprintf(stdout,"Node");
      break;
    default:
      fprintf(stderr, "WARN: unknown type!\n");
      break;
  }
  for ( ci_nr = item_min ; ci_nr < item_max ; ci_nr++ )
  {
    fprintf(stdout,",Item %d",ci_nr);
  }
  fprintf(stdout,"\n");
}

static void print_cpu(x86_adapt_device_type type, int cpu, int print_hex, int item_min, int item_max)
{
  int fd = x86_adapt_get_device_ro(type, cpu);
  uint64_t result;
  int ret;
  int ci_nr = 0;
  fprintf(stdout,"%d",cpu);
  if ( fd > 0 )
  {
    for ( ci_nr = item_min ; ci_nr < item_max ; ci_nr++ )
    {
      if ((ret = x86_adapt_get_setting(fd,ci_nr,&result)) != 8)
      {
        fprintf(stderr,"Could not read item %d for cpu/die %d\n",ci_nr,cpu);
        return;
      }
      if (print_hex)
        fprintf(stdout,",%"PRIx64,result);
      else
        fprintf(stdout,",%"PRIu64,result);
    }
  } else
  {
    fprintf(stderr,"Could not open /dev/x86_adapt/... file for cpu/die %d\n",cpu);
    exit(-1);
  }
  fprintf(stdout,"\n");
  x86_adapt_put_device(type, cpu);
}

int main(int argc, char ** argv)
{
  int cpu=-1;
  int hex=0;
  char c;
  int verbose=0;
  char * knob=NULL;
  int item_min=-1, item_max=-1, ci_nr;
  struct x86_adapt_configuration_item item;
  x86_adapt_device_type type = X86_ADAPT_CPU;

  while (1)
  {
    static struct option long_options[] =
      {
          {"help", no_argument, 0, 'h'},
          {"hex", no_argument, 0, 'H'},
          {"node", no_argument, 0, 'n'},
          {"cpu", required_argument, 0, 'c'},
          {"item", required_argument, 0, 'i'},
          {"verbose", no_argument, 0, 'v'},
          {0,0,0,0}
      };
    int option_index = 0;

    c = getopt_long(argc, argv, "c:i:hHnv", long_options, &option_index);
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


  if (knob != NULL)
  {
    int nr_items = x86_adapt_get_number_cis(type);
    /* get item index */
    for ( ci_nr = 0 ; ci_nr < nr_items ; ci_nr++ )
    {
      if (x86_adapt_get_ci_definition(type,ci_nr,&item))
      {
        fprintf(stderr,"Could not read item definition\n");
        exit(1);
      }
      if (strcmp(item.name,knob)==0)
      {
        item_min = ci_nr;
        item_max = ci_nr+1;
        break;
      }
    }
    /* not found? */
    if ( item_min == -1 )
    {
      fprintf(stderr,"The option %s is not available. Available options are listed now:\n",knob);
      print_cis(type, verbose,0,x86_adapt_get_number_cis(type));
      exit(1);
    }
  }
  else
  {
    item_min = 0;
    item_max = x86_adapt_get_number_cis(type);
  }

  print_cis( type, verbose, item_min, item_max );
  print_header( type, item_min, item_max );
  if ( cpu == -1 )
  {
    for ( cpu = 0 ; cpu < x86_adapt_get_nr_avaible_devices(type) ; cpu++ )
    {
      print_cpu( type, cpu, hex, item_min, item_max );
    }
  } else
  {
    print_cpu( type, cpu, hex, item_min, item_max );
  }
  

  return 0;
}
