shmtest
=======

Shared Memory Test inspired by problems @awebneck had with shared memory
```
Usage: shmtest [OPTION]
  --key=shmkey              Key to use for shm_get
  --address-file=filename   Store the shmaddr in filename for the next process
  --delay=n                 Delay for n secs after attachment before exiting
  --size=n                  Size (in bytes) of the segment - defaults to 4096
  --affinity                Attempt to set processor affinity to processor 0
  --verbose                 Print verbose messages
  --create                  Force a delete and create shared memory segment
  --help                    Print this message
```
shmtest will create a shared memory segment with key 123456 if no key is specified
and the shared memory segment is not already created.   If a shared memory segment
is already created, shmtest will attempt to attach to it with the address returned
to the original creator


Exit status is 0 if the attachment was successful, -1 if not
