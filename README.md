# About
libBitmapWag is a C library containing functions for reading and writing bitmaps 
programatically. This library makes a .so for dynamic linking and a .a for 
static linking. 

For an example of how to use this library, please see the file _bitmap.c_. 

This work is based on the description of the spec at 
www.fortunecity.com/skyscraper/windows/364/bmpffrmt.htm 

This library is meant to have simple dependencies, it relies only on the 
standard c libraries stdio.h, stdlib.h, and inttypes.h. 

# Building
Build process was designed for Linux and gcc. 

Run _make_ to build the software. 

To make a debugging version, for use in gdb, run _make debug_.

Run the example program by running _./bitmap_ 

Running _make install_ will put the contents of the include/ and lib/ 
directories into the /usr/local/ directory. 

