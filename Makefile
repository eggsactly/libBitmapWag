#  This file is part of libBitmapWag.
#
#  libBitmapWag is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  libBitmapWag is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with libBitmapWag.  If not, see <https://www.gnu.org/licenses/>.

# This Makefile is meant to compile 1 library, along with 1 example program
# Any source file that is meant to be part of the library should start with 
# the prefix `lib'. 

# Compiler 
CC?=gcc
# Installation location
PREFIX?=/usr/local

# name of example executable 
EXE:=bitmap
# name of the libary
LIBNAME:=libBitmapWag

# This make file will build every file it sees in the directory ending with a 
# .c
SRC_EXTENSION:=c
# Build process creates the folders obj, lib and inc for the .o, .so and .h 
# files respectively 
OBJ_DIR:=obj
LIB_DIR:=lib
INC_DIR:=include

# Character separating directory levels can be OS dependent
DIR_CHAR:=/

# All source files starting with lib are to be part of the library 
LIBSRCS:=$(wildcard lib*.$(SRC_EXTENSION))
LIBOBJS:=$(patsubst %.$(SRC_EXTENSION), $(OBJ_DIR)$(DIR_CHAR)%.o, $(LIBSRCS))
LIBHEADS:=$(wildcard lib*.h)
LIBINCS:=$(patsubst %, $(INC_DIR)$(DIR_CHAR)%, $(LIBHEADS))

# Filter out the source files for the example application from the library files
SOURCES:=$(filter-out $(LIBSRCS), $(wildcard *.$(SRC_EXTENSION)))
OBJECTS:=$(patsubst %.$(SRC_EXTENSION), $(OBJ_DIR)$(DIR_CHAR)%.o, $(SOURCES))

# Set compile flags
CFLAGS:=-fPIC -O3
INCFLAGS:=$(patsubst %, -I%, $(INC_DIR))

DEBUG:=

# Build the test exexutable, static, and dynamic libraries 
.PHONY: all
all: $(LIBINCS) $(LIB_DIR) lib/$(LIBNAME).a lib/$(LIBNAME).so $(EXE)

# The debug option cleans and builds the application with the -g compile flag
.PHONY: debug
debug: clean 
debug: DEBUG+=-g 
debug: all

# Link together the example application
$(EXE): $(OBJECTS) $(LIB_DIR)$(DIR_CHAR)$(LIBNAME).a
	$(CC) $^ -o $@

# Create .a 
$(LIB_DIR)$(DIR_CHAR)$(LIBNAME).a: $(LIBOBJS)
	ar rcs $@ $^

# Create .so
$(LIB_DIR)$(DIR_CHAR)$(LIBNAME).so: $(LIBOBJS)
	$(CC) $^ -shared -o $@

# Compile individual sources 
$(OBJ_DIR)$(DIR_CHAR)%.o: %.$(SRC_EXTENSION) $(OBJ_DIR) $(LIBINCS)
	$(CC) $(DEBUG) -c $(INCFLAGS) $(CFLAGS) $< -o $@ -DAPP_NAME=\"$(EXE)\"

# Copy header files to special inc directory and lock the files so that no one
# accidentally edits the copied include files 
$(INC_DIR)$(DIR_CHAR)%.h: %.h $(INC_DIR)
	cp -f $< $@
	chmod a-w $@

# Generate directories 
$(LIB_DIR):
	mkdir $(LIB_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)
		
$(INC_DIR):
	mkdir $(INC_DIR)

# Clean Directive, remove all generated files 
.PHONY: clean
clean:
ifeq ($(UNAME_S),Windows_NT) 
	DEL /F /s $(EXE) $(TESTDIR)$(DIR_CHAR)$(TEST_EXE)
	rd /q /s $(OBJ_DIR) $(LIB_DIR) $(INC_DIR)
else
	rm -rf $(EXE) $(OBJ_DIR) $(LIB_DIR) $(INC_DIR)
endif

# Install Directive, copy the library to the system 
.PHONY: install
install: all
	install $(LIB_DIR)$(DIR_CHAR)*.so $(PREFIX)$(DIR_CHAR)$(LIB_DIR)
	install $(INC_DIR)$(DIR_CHAR)*.h $(PREFIX)$(DIR_CHAR)$(INC_DIR)


