# FatFs test library

This small library can be used for testing some application which uses 
the Fatfs module from http://elm-chan.org/fsw/ff/00index_e.html.

It wraps the posix stdio functions to the FatFs API. It can be used on
any directory which can be mounted with the f_mount() function.


## Usage

```c
#include "ff.h"


int main(int argc, char *argv[])
{
  FATFS fs;
  
  if (f_mount(&fs, "/tmp/test1", 0) != FR_OK)
    return -1;
	
  FIL fil;
  if (f_open(&fil, "file1.txt", FA_READ) != FR_OK)
    return -1;
  
  char ln[256];
  while (f_gets(line, sizeof(ln), &fil)) {
    printf(ln);
  }
  
  f_close(&fil);
  
  return 0;
}
    
```

## Literature

* [FatFs](http://elm-chan.org/fsw/ff/00index_e.html)
