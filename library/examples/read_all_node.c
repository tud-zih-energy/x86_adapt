#include <stdio.h>
#include "x86_adapt.h"
#include <string.h>


void main()
{
  int i,j,nr_cis;
  x86_adapt_init();
  struct x86_adapt_avaible_devices* nbs=get_avaible_dies();
  nr_cis=x86_adapt_get_number_cis(X86_ADAPT_DIE);
  
  struct x86_adapt_configuration_item * item;
  
  for (i=0;i<nbs->count;i++){
    int fd=x86_adapt_get_device_ro(X86_ADAPT_DIE, i);
    
    fprintf(stdout,"CPU %i\n",i);
    for (j=0;j<nr_cis;j++){
      unsigned long long result=77;
      x86_adapt_get_ci_definition(X86_ADAPT_DIE,j,&item);
      fprintf(stdout,"Setting name=%s description:%s length=%d\n",item->name,item->description,item->length);
      x86_adapt_get_setting(fd,j,&result);
      fprintf(stdout,"Setting %i %s = %llu\n",j,item->name,result);
    }
    x86_adapt_put_device(X86_ADAPT_DIE, i);
  }
}
