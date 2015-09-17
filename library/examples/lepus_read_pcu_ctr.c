#include <stdio.h>
#include "x86_adapt.h"
#include <string.h>


#define NR 100000000

unsigned long long timer[NR];
unsigned long long tsc[NR];

#define sync_rdtsc1(val) \
   do {\
      unsigned int cycles_low, cycles_high;\
      asm volatile ("CPUID\n\t"\
      "RDTSC\n\t"\
      "mov %%edx, %0\n\t"\
      "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx"); \
      (val) = ((unsigned long) cycles_low) | (((unsigned long) cycles_high) << 32);\
   } while (0)

void main()
{
  unsigned long long i,j,nr_cis,last=0;
  x86_adapt_init();
  struct x86_adapt_avaible_devices* nbs=get_avaible_dies();
  nr_cis=x86_adapt_get_number_cis(X86_ADAPT_DIE);
  
  struct x86_adapt_configuration_item * item;
  
  int fd=x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
    
  unsigned long long result=77;
  x86_adapt_get_ci_definition(X86_ADAPT_DIE,0,&item);
  fprintf(stdout,"Setting name=%s description:%s length=%d\n",item->name,item->description,item->length);
  
  for (i=0;i<NR;i++){
    timer[i]=77;
    x86_adapt_get_setting(fd,0,&timer[i]);
    sync_rdtsc1(tsc[i]);
  }
  
  for (i=1;i<NR;i++){
    if (timer[i]-timer[i-1])
      last=i;
    fprintf(stdout,"%llu %llu %llu %llu\n",(tsc[i]-tsc[0])/2500,timer[i]-timer[0],timer[i]-timer[i-1],i-last);
//      last=i;
//      fprintf(stdout,"last=%d\n",last);
  }
  x86_adapt_put_device(X86_ADAPT_DIE, 0);
}
