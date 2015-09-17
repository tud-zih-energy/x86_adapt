#include <stdio.h>
#include "x86_adapt.h"
#include <string.h>


int main(int argc, char ** argv)
{
  int i,j,nr_cis,ret;
  x86_adapt_init();
  struct x86_adapt_avaible_devices* cpus=get_avaible_cpus();
  nr_cis=x86_adapt_get_number_cis(X86_ADAPT_CPU);
  
  struct x86_adapt_configuration_item * item;
  if (argc!=2){
    fprintf(stderr,"Usage: set_perf_bias [0|6|15]\n");
    return 1;
  }
  unsigned long long setting=atoi(argv[1]);
    
  for (j=0;j<nr_cis;j++){

    x86_adapt_get_ci_definition(X86_ADAPT_CPU,j,&item);

    if (!strstr(item->name,"Intel_Energy_Perf_Bias"))
      continue;

    for (i=0;i<cpus->count;i++){
      int fd=x86_adapt_get_device(X86_ADAPT_CPU, i);
      ret=x86_adapt_set_setting(fd,j,setting);
      x86_adapt_put_device(X86_ADAPT_CPU, i);
    }
  }
  return 0;
}
