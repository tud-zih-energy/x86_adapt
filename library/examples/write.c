#include <stdio.h>
#include "x86_adapt.h"
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

void print_help(char ** argv)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s [ %s-ARGS ...] \"COMMAND [ ARGS ...]\"\n", argv[0], argv[0]);
  fprintf(stderr, "\n");
  fprintf(stderr, "%s-ARGS:\n",argv[0]);
  fprintf(stderr, "\t -h --help: print this help\n");
  fprintf(stderr, "\t -H --hex: set in hexadecimal form\n");
  fprintf(stderr, "\t -d --die: set die options instead of CPU options\n");
  fprintf(stderr, "\t -c --cpu: Use this CPU (default=all)\n"
                                "If -d is set, use the die with this nr.\n");
  fprintf(stderr, "\t -o --option: set this option\n");
  fprintf(stderr, "\t -V --value: set to this value\n");
  fprintf(stderr, "\t -v --verbose: print more information\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "This program reads all available CPU knobs from x86_adapt.\n");
}

void print_cis(x86_adapt_device_type type, int verbose)
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

void set_cpu(x86_adapt_device_type type, int cpu, char* knob, uint64_t new_setting,int verbose)
{
  struct x86_adapt_configuration_item item;
  int nr_items = x86_adapt_get_number_cis(type);
  int fd = x86_adapt_get_device(type, cpu);
  int ret;
  int ci_nr = 0;
  if ( fd > 0 )
  {
    int ci_found = 0;
    for ( ci_nr = 0 ; ci_nr < nr_items ; ci_nr++ )
    {
      if (x86_adapt_get_ci_definition(type,ci_nr,&item))
      {
        fprintf(stderr,"Could not read item definition\n");
        exit(1);
      }
      if (strcmp(item.name,knob)==0)
      {
        ci_found = 1;
        if  (verbose)
          fprintf(stdout,"Set definition %s on cpu/die %d to %"PRIi64"\n",knob,cpu,new_setting);
        if ((ret = x86_adapt_set_setting(fd,ci_nr,new_setting)) != 8)
        {
          fprintf(stderr,"Could not set definitions for cpu/die %d\n",cpu);
          exit(1);
        }
      }
    }
    if ( !ci_found )
    {
      fprintf(stderr,"The option %s is not available. Available options are listed now:\n",knob);
      print_cis(type, verbose);
      exit(1);
    }
  } else
  {
    fprintf(stderr,"Could not open /dev/x86_adapt/... file for cpu/die %d\n",cpu);
    exit(-1);
  }
  x86_adapt_put_device(type, cpu);
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


  while (1)
  {
    static struct option long_options[] =
      {
          {"help", no_argument, 0, 'h'},
          {"hex", no_argument, 0, 'H'},
          {"option", required_argument, 0, 'o'},
          {"value", required_argument, 0, 'V'},
          {"die", no_argument, 0, 'd'},
          {"cpu", required_argument, 0, 'c'},
          {"verbose", no_argument, 0, 'v'},
          {0,0,0,0}
      };
    int option_index = 0;

    c = getopt_long(argc, argv, "hHc:o:V:dv", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 'h':
      print_help(argv);
      return 1;
    case 'H':
      hex = 1;
      break;
    case 'd':
      type = X86_ADAPT_DIE;
      break;
    case 'c':
      cpu = atoll(optarg);
      break;
    case 'o':
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

  if ( hex )
    new_setting=strtoll(new_setting_str,NULL,16);
  else
    new_setting=strtoll(new_setting_str,NULL,10);

  if ( cpu == -1 )
  {
    for ( cpu = 0 ; cpu < x86_adapt_get_nr_avaible_devices(type) ; cpu++ )
    {
      set_cpu(type, cpu, knob, new_setting,verbose);
    }
  } else
  {
    set_cpu(type, cpu, knob, new_setting,verbose);
  }
}
