#include <stdio.h>
#include <x86_adapt.h>
#include <string.h>

/*
 * Compile with gcc print_all_cpu_settings_example.c -lx86_adapt
 * */

/* should be int main (int argc, char ** argv) */
void main()
{
  int i,j,nr_cis;
  /* initialize library */
  x86_adapt_init();
  /* get cpus */
  struct x86_adapt_avaible_devices* cpus=get_avaible_cpus();

  /* get number of settings */
  nr_cis=x86_adapt_get_number_cis(X86_ADAPT_CPU);

  /* a certain setting */
  struct x86_adapt_configuration_item * item;
  
  /* for all cpus: print all settings */
  for (i=0;i<cpus->count;i++){
    /* open x86_device for cpu i */
    int fd=x86_adapt_get_device_ro(X86_ADAPT_CPU, i);
    
    fprintf(stdout,"CPU %i\n",i);
    /* print all settings */
    for (j=0;j<nr_cis;j++){
      unsigned long long result=77;
      /* get definition (for name, description, ...) of item j */
      x86_adapt_get_ci_definition(X86_ADAPT_CPU,j,&item);
      /* get setting of item j */
      if (x86_adapt_get_setting(fd,j,&result)==8){
        /* print information of item j */
        fprintf(stdout,"Setting %i %s = %llu\n",j,item->name,result);
      } else {
        /* error while reading */
        fprintf(stdout,"Error while reading\n");
      }
    }
    /* put device for cpu i */
    x86_adapt_put_device(X86_ADAPT_CPU, i);
  }
}
