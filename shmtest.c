#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <getopt.h>


/* atexit function detach from the memory */
void detach_the_memory();
void *mem_to_detach = NULL;


/* fetch the pointer to use for shmid */
void * fetch_pointer(int shmid);
/* store the pointer if we created the shared mem */
int store_pointer(void *shared_memory_pointer);
int process_parameters(int argc, char *argv[]);


#define GET_ADDRESS_FROM_FILE


void usage();

/*
 * Command line parameters
 */

key_t shm_key = 123456;
char address_file[256] = "pointer_storage.txt";
int set_affinity = 0;
int verbose = 0;
int force_create = 0;
int pause_seconds = 0;
size_t sz = 4096;

main (int argc, char *argv[]) {

  void *shmaddr;
  void *requested_addr = NULL;
  void *usable_addr = NULL;
  int i=0;
  size_t usable_size = 0;
  int shm_created = 0;
  int shmid;

  process_parameters(argc,argv);

  if(set_affinity) {
    cpu_set_t *cpusetp;
    cpusetp = CPU_ALLOC(8);
    CPU_ZERO(cpusetp);
    CPU_SET(1,cpusetp);
    sched_setaffinity(0,9,cpusetp);
  }

  usable_size = sz - sizeof(void *);

  atexit( detach_the_memory );

  shmid = shmget(shm_key, sz, 0666);

  if(shmid < 0 || force_create) {
    printf("Dang, do I really have to create this?\n");
    shmid = shmget(shm_key, 0,0666);
    if(shmid > 0) {
      printf("Probably wrong sized segment - deleting %d\n",shmid);
      shmctl(shmid,IPC_RMID,NULL);
      shmid = shmget(shm_key, sz,IPC_CREAT | 0666);
      printf("Created %d with size %d\n",shmid,sz);
    } else {
      printf("Dangit!  Couldn't find the original...\n");
      shmid = shmget(shm_key, sz,IPC_CREAT | 0666);
      printf("Created %d with size %d\n",shmid,sz);
      exit(1);
    }
    shm_created = 1;
  }

  if(shmid < 0) {
    printf("********************************** Something really bad happened\n");
    exit(1);
  }

  if(!shm_created) {
    requested_addr = fetch_pointer(shmid);
  } else {
    requested_addr = NULL;
  }
  printf("Requesting address - %p",requested_addr);
/*
  printf("requested_addr - %p\n",requested_addr);
  printf("shmaddr - %p\n",shmaddr);
  printf("usable_address - %p\n",usable_addr);
  printf("usable_size - %d\n",usable_size);
*/

  shmaddr = (void *)-1;
  mem_to_detach = shmaddr = shmat(shmid, requested_addr, 0666);
  usable_addr = shmaddr + sizeof(void *);
  if(shmaddr == (void *)0xffffffff || (requested_addr && (requested_addr != shmaddr))) {
      printf(" --- Failure! shmaddr was %p instead\n",shmaddr,requested_addr);
      exit(1);
    } else {
      printf(" --- Success address is %p\n",shmaddr);
    }

  if( shm_created || !requested_addr ) {
      store_pointer(shmaddr);
    }

  if(!shm_created) {
    unsigned char *cp = usable_addr;
    for(i=0;i<usable_size;i++) {
      if(!(i % 1024)) {
/*        printf("comparing bytes %d     \r",i); */
      }
      if(*cp != (unsigned char)i) {
/*        printf("Dammit! %d != %d\n",*cp,((unsigned char)i)); */
      } else {
        cp++;
      }
    }
/*    printf("\nCompare complete %d     \n",i); */
  } else {
    unsigned char *cp = usable_addr;
    for(i=0;i<usable_size;i++) {
      *cp++ = (unsigned char)i;
      if(!(i % 1024)) {
/*         printf("loading bytes %d     \r",i); */
      }
    }
/*     printf("\nLoading complete %d     \n",i); */
  }
  if(pause_seconds) {
    sleep(pause_seconds);
  }
  exit(0);
}



int store_pointer(void *shared_memory_pointer) {
#ifdef GET_ADDRESS_FROM_FILE
  FILE *fp;
  fp = fopen("./pointer_storage.txt","w");
  if(!fp) {
      printf("Couldn't open the file!\n");
    }
  fprintf(fp,"%p",shared_memory_pointer);
  printf("Wrote this ------>  %p\n",shared_memory_pointer);
 fclose(fp);
#else
 void **stored_pointer = NULL;
 *stored_pointer = shared_memory_pointer;
#endif
  return 0;
}


void * fetch_pointer(int shmid) {
  void *shared_memory_pointer = NULL;
#ifdef GET_ADDRESS_FROM_FILE
  FILE *fp;
  fp = fopen("pointer_storage.txt","r");
  if(!fp) {
      printf("Couldn't open the file!\n");
  } else {
    fscanf(fp,"%p",&shared_memory_pointer);
    fclose(fp);
  }
#else
  void **stored_pointer = NULL;
  shmaddr = shmat(shmid, NULL, 0666);
  stored_pointer = shmaddr;
#endif
  return shared_memory_pointer;
}


int process_parameters(int argc, char *argv[]) {
  int c;
  int digit_optind = 0;

  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"key",          required_argument, 0,  'k' },
      {"address-file", required_argument, 0,  'f' },
      {"delay",        required_argument, 0,  'd' },
      {"size",         required_argument, 0,  's' },
      {"affinity",     no_argument,       0,  'a' },
      {"verbose",      no_argument,       0,  'v' },
      {"create",       no_argument,       0,  'c' },
      {"help",         no_argument,       0,  'h' },
      {0,              0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "d:s:k:f:avch", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 0:
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;
    case 'k':
      shm_key = atoi(optarg);
      break;
    case 'd':
      pause_seconds = atoi(optarg);
      break;
    case 's':
      sz = atoi(optarg);
      break;
    case 'f':
      strcpy(address_file,optarg);
      printf("option a with optarg %s\n",optarg);
      break;
    case 'a':
      set_affinity = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'c':
      force_create = 1;
      break;
    case 'h':
      usage();
      break;
    case '?':
      break;
    default:
      printf("?? getopt returned character code 0%o ??\n", c);
    }
  }

  if (optind < argc) {
    printf("ignoring non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }

  return 0;
}


void usage() {
  printf("\
Usage: shmtest [OPTION]\n\
  --key=shmkey              Key to use for shm_get\n\
  --address-file=filename   Store the shmaddr in filename for the next process\n\
  --delay=n                 Delay for n secs after attachment before exiting\n\
  --size=n                  Size (in bytes) of the segment - defaults to 4096\n\
  --affinity                Attempt to set processor affinity to processor 0\n\
  --verbose                 Print verbose messages\n\
  --create                  Force a delete and create shared memory segment\n\
  --help                    Print this message\n\
\n\
shmtest will create a shared memory segment with key 123456 if no key is specified\n\
and the shared memory segment is not already created.   If a shared memory segment\n\
is already created, shmtest will attempt to attach to it with the address returned\n\
to the original creator\n\
\n\
Exit status is 0 if the attachment was successful, -1 if not\n\
"
);
exit(0);
}



void detach_the_memory() {
  if(mem_to_detach) {
/*    printf("Detaching from %p\n",mem_to_detach); */
    shmdt(mem_to_detach);
  }
}


