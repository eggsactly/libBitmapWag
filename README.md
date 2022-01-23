# About
libBitmapWag allows you to programatically read, edit, and write bitmap images 
using an extern C interface.

libBitmapWag is a C library containing functions for reading and writing bitmaps 
programatically. This library makes a .so for dynamic linking and a .a for 
static linking. 

For an example of how to use this library, please see the example/unit test 
file _bitmap.c_. 

This work is based on the description of the spec at 
www.fortunecity.com/skyscraper/windows/364/bmpffrmt.htm 

This library is meant to have simple dependencies, it relies only on the 
standard c libraries stdio.h, stdlib.h, and inttypes.h. 

This library has been tested on x86 and has not been tested on a Big Endian 
architecture. 

# Building
Build process was designed for Linux and gcc. 

To build the software run:
```
make
```

To make a debugging version, for use in gdb, run 
```
make debug
```

Run the example program by running 
```
./bitmap
```

To install, which will put the contents of the include/, lib/, and bin/
directories into the /usr/local/ directory, run:
```
make install
```

After installation you will need to make sure that /usr/local/bin is in your
LD\_LIBRARY\_PATH so that the dynamic linker can find the library when you 
run code that links to it. 
```
echo $LD_LIBRARY_PATH
```

If the path does not get returned, then you will need to add the following 
line to your _.profile_ file in your /home/username area. 
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/bin
```

Either source your .profile file or log out and log back in and then you will 
be ready to link to the library. 

# Coding Style Guidelines 

1. Tabs are four spaces (except in makefiles). 
2. Line width is limited to 80 characters. 
3. No printfs in the library code, functions are to only return an error code.
4. Function implementations shall only be in the .c files.
5. All source files shall have the GPL copyright banner at the top.

