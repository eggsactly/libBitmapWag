//  This file is part of libBitmapWag.
//
//  libBitmapWag is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  libBitmapWag is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with libBitmapWag.  If not, see <https://www.gnu.org/licenses/>.

// bitmap.c is a basic unit test for the libBitmapWag library. 

#include <stdio.h>

#include "BitmapWag.h"

// APP_NAME is used for stderr outputs in this program
// it should be provided by the makefile, if not, it is redfined here. 
#ifndef APP_NAME
    #define APP_NAME "bitmap"
#endif

// Main function is the start of the program 
int main()
{
    // Declare two bitmap image structs, img for writing, img2 for reading 
    BitmapWagImg *img, *img2;

    // BitmapWagError is an enum containing error codes
    BitmapWagError error;
    
    // Bitmap dimensions 
    const unsigned width = 8;
    const unsigned height = 8;

    // Valid pixel sizes are 1, 2, 4, 8, 16, 24
    const unsigned bitsPerPixel = 1;

    // This is the name of the output file this program writes 
    const char outputFile[] = "checker-board.bmp";

    fprintf(stderr, "%s: info: BitmapWag API version: %d.%d.%d\n", APP_NAME, 
            MajorVersionBitmapWag(), MinorVersionBitmapWag(), 
            PatchVersionBitmapWag());

    // Construct the bitmap 
    img = ConstructBitmapWag();

    // Initialize a new bitmap
    error = InitializeBitmapWag(img, height, width, bitsPerPixel);

    if(error)
    {
        fprintf(stderr, "%s: error: InitializeBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }

    // Write a checkerboard
    for(unsigned i = 0; i < width; i++)
    {
        for(unsigned j = 0; j < height; j++)
        {
            error = SetBitmapWagPixel(img, i, j, 
                ((i+j) & 1) * 0xFF, ((i+j) & 1) * 0xFF, ((i+j) & 1) * 0xFF);

            if(error)
            {
                fprintf(stderr, "%s: error: SetBitmapWagPixel %d %d: %s.\n", 
                    APP_NAME, i, j, ErrorsToStringBitmapWag(error));
                return -1;
            }
        }
    }

    // Write the bitmap to the file 
    error = WriteBitmapWag(img, outputFile);
    if(error)
    {
        fprintf(stderr, "%s: error: WriteBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
    }
    
    fprintf(stderr, "%s: info: %s written.\n", APP_NAME, outputFile);

    // Unallocate memory tied to the bitmap 
    error = FreeBitmapWag(img);
    img = NULL; 

    if(error)
    {
        fprintf(stderr, "%s: error: FreeBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }

    // Construct the bitmap 
    img2 = ConstructBitmapWag();

    // Try reading the bitmap that was just written
    error = ReadBitmapWag(img2, outputFile);
    if(error)
    {
        fprintf(stderr, "%s: error: ReadBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }
 
    fprintf(stderr, "%s: info: Bitmap: %s opened.\n", APP_NAME, 
            outputFile);

    // Print bitmap information 
    fprintf(stderr, "%s: info: Bitmap dimensions: %dx%d\n", APP_NAME, 
            GetBitmapWagWidth(img2), GetBitmapWagHeight(img2));

    // Unallocate memory tied to the bitmap 
    error = FreeBitmapWag(img2);
    img2 = NULL;

    if(error)
    {
        fprintf(stderr, "%s: error: FreeBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }

    return 0; 
}

