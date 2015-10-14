
#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>

#include <x86_adapt.h>

struct fd_client_count {
	int fd;
	int clients;
    pthread_mutex_t mutex;
}__attribute__((aligned(64)));

static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct x86_adapt_configuration_item ** config_items=NULL;
static unsigned int config_items_length[]={0,0};

static struct fd_client_count ** fds ;
static struct fd_client_count * fd_all;
static unsigned int fds_length[] = {0,0};

static int initialized=0;

/* gets the configuration for each avaible x86_adapt configuration item */
static int get_configuration_items(int fd, struct x86_adapt_configuration_item ** entries)
{
	int size_read;
	ssize_t bytes_read;
	char * data;
	char * read_data;
	int entries_length=0;
	/* read size to read */
	bytes_read = pread(fd, &size_read, 4, 0);
	if (bytes_read!=4) {
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "x86_adapt: Wrong definition size: %zi\n", bytes_read);
		close(fd);
		return -EIO;
	}
    data = alloca(size_read * sizeof(char));
	bytes_read=pread(fd,data,size_read,0);
	if (bytes_read < 0 || (unsigned int)bytes_read!=size_read) {
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "x86_adapt: Error reading definitions: %zi %i!\n",bytes_read, size_read);
		close(fd);
		return -EIO;
	}
	read_data=data;
	read_data+=4;
	while (read_data<bytes_read+data) {
        int length, name_length, descr_length;
        char *name, *descr;
		/* skip 4 byte id */
		read_data+=4;
		/* 1 byte length */
		 length=read_data[0];
		read_data++;
		/* 4 byte name_length */
		 name_length=((int *)read_data)[0];
		read_data+=4;
		/* name_length byte name */
		name=strndup(read_data,name_length);
		read_data+=name_length;
		/* 4 byte descr. length */
		descr_length=((int *)read_data)[0];
		read_data+=4;
		/* descr_length byte descr. */
		descr=strndup(read_data,descr_length);
		read_data+=descr_length;
		*entries=realloc(*entries,sizeof(struct x86_adapt_configuration_item)*(entries_length+1));
		if (*entries==NULL) {
            fprintf(stderr, "ERROR\n");
            fprintf(stderr, "x86_adapt: Failed to realloc memory for x86_adapt configuration items\n");
			exit(-1);
        }
		(*entries)[entries_length].name=name;
		(*entries)[entries_length].description=descr;
		(*entries)[entries_length].length=length;
		entries_length++;
	}
	return entries_length;
}

/* gets the of avaible cpus or dies */
int __get_avaible(char * path)
{
    int n, i;
    char *end;
    struct dirent **namelist;
    int count = 0;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR\n");
        fprintf(stderr, "x86_adapt: failed to open '%s'!\n",path);
        return fd;
    }
    close(fd);

    
    /* count avaible devices */
    n = scandir(path, &namelist, NULL, versionsort);

    for(i=0;i<n;i++) {
        strtol(namelist[i]->d_name, &end, 10);
        if(! *end) {
            count++;
        }
    }
    return count;
}

int x86_adapt_get_nr_avaible_devices(x86_adapt_device_type device_type)
{
    switch (device_type)
    {
        case X86_ADAPT_CPU:
            return __get_avaible("/dev/x86_adapt/cpu");
        case X86_ADAPT_DIE:
            return __get_avaible("/dev/x86_adapt/node");
        default:
            return -1;
    }
}

/* This should initialize the library and allocate data structures */
int x86_adapt_init(void)
{
    int fd;
    unsigned int i, j;
    pthread_mutex_lock(&init_mutex);
    if (initialized) {
        pthread_mutex_unlock(&init_mutex);
        return 0;
    }
	/* initialize the configuration_items */
	config_items=calloc(2,(sizeof(void*)));
    if(!config_items)
        return -ENOMEM;
	fd = open("/dev/x86_adapt/cpu/definitions", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "x86_adapt: failed to open '/dev/x86_adapt/cpu/definitions': %i!\n",fd);
		return -EIO;
	}
    config_items_length[X86_ADAPT_CPU]=get_configuration_items(fd, &(config_items[X86_ADAPT_CPU]));
	close(fd);
	/* initialize the configuration_items */
	fd = open("/dev/x86_adapt/node/definitions", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "x86_adapt: failed to open '/dev/x86_adapt/node/definitions': %i!\n",fd);
		/* why do the close here if the open failed? */
		close(fd);
		return -EIO;
	}
    config_items_length[X86_ADAPT_DIE]=get_configuration_items(fd, &(config_items[X86_ADAPT_DIE]));
	close(fd);

    fd_all=calloc(2,sizeof(struct fd_client_count));
    if (!fd_all)
        return -ENOMEM;
    
    fds=malloc(2*sizeof(void *));
    if (!fds)
        return -ENOMEM;

    /* TODO: besseren wert einlesen */
    fds_length[X86_ADAPT_CPU]=sysconf(_SC_NPROCESSORS_CONF);
    fds_length[X86_ADAPT_DIE]=sysconf(_SC_NPROCESSORS_CONF);

    fds[X86_ADAPT_CPU]=calloc(fds_length[X86_ADAPT_CPU],sizeof(struct fd_client_count));
    if (!fds[X86_ADAPT_CPU])
        return -ENOMEM;
    fds[X86_ADAPT_DIE]=calloc(fds_length[X86_ADAPT_DIE],sizeof(struct fd_client_count));
    if (!fds[X86_ADAPT_DIE])
        return -ENOMEM;

    /* init local mutexes */
    for(i=0;i<2;i++) {
        pthread_mutex_init(&fd_all[i].mutex, NULL);
        for(j=0;j<fds_length[i];j++)
            pthread_mutex_init(&fds[i][j].mutex, NULL);
    }

	initialized=1;
    pthread_mutex_unlock(&init_mutex);
	return 0;
}

/* returns file descriptor for /dev/x86_adapt/<cpu|node>/<nr>*/
int x86_adapt_get_device(x86_adapt_device_type device_type, uint32_t nr)
{
	if (!initialized)
        return -EPERM;
	if (device_type > 1)
        return -ENXIO;
	if (nr >= fds_length[device_type])
        return -ENXIO;
	/* are we the first client? */
    pthread_mutex_lock(&fds[device_type][nr].mutex);
	if (fds[device_type][nr].clients==0) {
		/* open the fd */
		char buffer[256];
		sprintf(buffer,"/dev/x86_adapt/%s/%i",device_type==0?"cpu":"node",nr);
		fds[device_type][nr].fd=open(buffer, O_RDWR);
	}
	/* increase nr of clients */
	fds[device_type][nr].clients++;
  pthread_mutex_unlock(&fds[device_type][nr].mutex);
	return fds[device_type][nr].fd;
}

/* returns file descriptor for /dev/x86_adapt/<cpu|node>/<nr> in read only mode*/
int x86_adapt_get_device_ro(x86_adapt_device_type device_type, uint32_t nr)
{
  if (!initialized)
        return -EPERM;
  if (device_type > 1)
        return -ENXIO;
  if (nr >= fds_length[device_type])
        return -ENXIO;
  /* are we the first client? */
    pthread_mutex_lock(&fds[device_type][nr].mutex);
  if (fds[device_type][nr].clients==0) {
    /* open the fd */
    char buffer[256];
    sprintf(buffer,"/dev/x86_adapt/%s/%i",device_type==0?"cpu":"node",nr);
    fds[device_type][nr].fd=open(buffer, O_RDONLY);
  }
  /* increase nr of clients */
  fds[device_type][nr].clients++;
  pthread_mutex_unlock(&fds[device_type][nr].mutex);
  return fds[device_type][nr].fd;
}

/* closes file descriptor */
int x86_adapt_put_device(x86_adapt_device_type device_type, uint32_t nr)
{
	if (!initialized)
        return -EPERM;
	if (device_type > 1)
        return -ENXIO;
	if (nr >= fds_length[device_type])
        return -ENXIO;
    pthread_mutex_lock(&fds[device_type][nr].mutex);
	/* are we the last client? */
	if (fds[device_type][nr].clients==1) {
		/* close the fd */
		close(fds[device_type][nr].fd);
	}
	/* decrease nr of clients */
	fds[device_type][nr].clients--;
    pthread_mutex_unlock(&fds[device_type][nr].mutex);
	return 0;
}


/* returns file descriptor for /dev/x86_adapt/all */
int x86_adapt_get_all_devices_ro(x86_adapt_device_type device_type) {
  if (!initialized)
        return -EPERM;
  if (device_type > 1)
        return -ENXIO;
    pthread_mutex_lock(&fd_all[device_type].mutex);
  /* are we the first client? */
  if (fd_all[device_type].clients==0) {
    /* open the fd */
    char buffer[256];
    sprintf(buffer,"/dev/x86_adapt/%s/all",device_type==0?"cpu":"node");
    fd_all[device_type].fd=open(buffer, O_RDONLY);
  }
  /* increase nr of clients */
  fd_all[device_type].clients++;
    pthread_mutex_unlock(&fd_all[device_type].mutex);
  return fd_all[device_type].fd;
}

/* returns file descriptor for /dev/x86_adapt/all */
int x86_adapt_get_all_devices(x86_adapt_device_type device_type) {
	if (!initialized)
        return -EPERM;
	if (device_type > 1)
        return -ENXIO;
    pthread_mutex_lock(&fd_all[device_type].mutex);
	/* are we the first client? */
	if (fd_all[device_type].clients==0) {
		/* open the fd */
		char buffer[256];
		sprintf(buffer,"/dev/x86_adapt/%s/all",device_type==0?"cpu":"node");
		fd_all[device_type].fd=open(buffer, O_RDWR);
	}
	/* increase nr of clients */
	fd_all[device_type].clients++;
    pthread_mutex_unlock(&fd_all[device_type].mutex);
	return fd_all[device_type].fd;
}

/* closes file descriptor */
int x86_adapt_put_all_devices(x86_adapt_device_type device_type)
{
	if (!initialized)
        return -EPERM;
	if (device_type > 1)
        return -ENXIO;
  pthread_mutex_lock(&init_mutex);
  pthread_mutex_lock(&fd_all[device_type].mutex);
	/* are we the last client? */
	if (fd_all[device_type].clients==1) {
		/* close the fd */
		close(fd_all[device_type].fd);
	}
	/* increase nr of clients */
	fd_all[device_type].clients--;
    pthread_mutex_unlock(&fd_all[device_type].mutex);
  pthread_mutex_unlock(&init_mutex);
	return 0;
}

/* returns the defintion of cpu/node x86_adapt configuration item */
int x86_adapt_get_ci_definition(x86_adapt_device_type device_type, uint32_t id, struct x86_adapt_configuration_item * item)
{
	if (!initialized)
        return -EPERM;
	if (device_type >= X86_ADAPT_MAX)
        return -ENXIO;
	if (id>=config_items_length[device_type])
        return -ENXIO;
	*item=config_items[device_type][id];
	return 0;
}

/* returns number of cpu/die configuration items */
int x86_adapt_get_number_cis(x86_adapt_device_type device_type)
{
	if (!initialized)
        return -EPERM;
	if (device_type >= X86_ADAPT_MAX)
        return -ENXIO;
	return config_items_length[device_type];
}

/* returns the index number of the supplied configuration item */
int x86_adapt_lookup_ci_name(x86_adapt_device_type device_type, const char * name)
{
    int i;
    int number_cis = x86_adapt_get_number_cis(device_type);
    if (number_cis<0)
        return number_cis;
    for (i=0;i<number_cis;i++) {
        size_t size = strlen(config_items[device_type][i].name);
        if(!strncmp(config_items[device_type][i].name, name, size))
            return i;
    }
    return -ENXIO;
}

/*  for a specific configuration item */
int x86_adapt_get_setting(int fd, int id, uint64_t * setting)
{
	if (!initialized)
        return -EPERM;
	/* pread at id */
	return pread(fd,setting,8,id);
}
/*  for a specific configuration item */
int x86_adapt_set_setting(int fd, int id, uint64_t setting)
{
	if (!initialized)
        return -EPERM;
	/* pread at id */
	return pwrite(fd,&setting,8,id);
}

/* This should finalize the library, close fd's and free data structures */
void x86_adapt_finalize(void)
{
	unsigned int j,i;
    pthread_mutex_lock(&init_mutex);
	if (!initialized)
        return;
	for (i=0;i<2;i++) {
		for (j=0;j<fds_length[i];j++) {
			if (fds[i][j].clients!=0)
				close(fds[i][j].fd);
            pthread_mutex_destroy(&fds[i][j].mutex);
        }
        for (j=0;j<config_items_length[i];j++) {
            free(config_items[i][j].name);
            free(config_items[i][j].description);
        }
		free(fds[i]);
		free(config_items[i]);
        pthread_mutex_destroy(&fd_all[i].mutex);
	}
	free(fds);
    free(fd_all);
	free(config_items);
	initialized=0;
    pthread_mutex_unlock(&init_mutex);
    return;
}
