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

#include <stdio.h>

#include "libBitmapWag.h"

// APP_NAME is used for stderr outputs in this program
// it should be provided by the makefile, if not, it is redfined here. 
#ifndef APP_NAME
    #define APP_NAME "bitmap"
#endif

// Main function is the start of the program 
int main()
{
    // Declare two bitmap image structs, img for writing, img2 for reading 
    BitmapWagImg img, img2;

    // BitmapWagError is an enum containing error codes
    BitmapWagError error;

    // colorUsed is an array used to keep track of which colors in the color
    // palette are being used to 
    uint8_t colorUsed[256] = {0};
    
    // Bitmap dimensions 
    const unsigned width = 8;
    const unsigned height = 8;

    // Valid pixel sizes are 1, 2, 4, 8, 16, 24
    const unsigned bitsPerPixel = 4;

    // This is the name of the output file this program writes 
    const char outputFile[] = "test.bmp";

    fprintf(stderr, "%s: info: BitmapWag API version: %d.%d.%d\n", APP_NAME, 
            MajorVersionBitmapWag(), MinorVersionBitmapWag(), 
            PatchVersionBitmapWag());

    // Initialize a new bitmap
    error = InitializeBitmapWag(&img, height, width, bitsPerPixel);
    if(error)
    {
        fprintf(stderr, "%s: error: InitializeBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }

    // Initialized the color used array 
    if(bitsPerPixel <= 8)
    {
        error = SetColorUsedArrayBitmapWag(&img, colorUsed);

        if(error)
        {
            fprintf(stderr, "%s: error: SetColorUsedArrayBitmapWag: %s.\n", 
                APP_NAME, ErrorsToStringBitmapWag(error));
            return -1;
        }
    }

    // Write a black and white gradient from left to right
    for(unsigned i = 0; i < width; i++)
    {
        for(unsigned j = 0; j < width; j++)
        {
            error = SetBitmapWagPixel(&img, i, j, i * 32, i * 32, i * 32, 
                colorUsed);

            if(error)
            {
                fprintf(stderr, "%s: error: SetBitmapWagPixel %d %d: %s.\n", 
                    APP_NAME, i, j, ErrorsToStringBitmapWag(error));
                return -1;
            }
        }
    }

    // Write the bitmap to the file 
    error = WriteBitmapWag(&img, outputFile);
    if(error)
    {
        fprintf(stderr, "%s: error: WriteBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
    }
    
    fprintf(stderr, "%s: info: %s written.\n", APP_NAME, outputFile);

    // Unallocate memory tied to the bitmap 
    error = FreeBitmapWag(&img);
    if(error)
    {
        fprintf(stderr, "%s: error: FreeBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }


    // Try reading the bitmap that was just written
    error = ReadBitmapWag(&img2, outputFile);
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
            GetBitmapWagWidth(&img2), GetBitmapWagHeight(&img2));

    // Unallocate memory tied to the bitmap 
    error = FreeBitmapWag(&img2);
    if(error)
    {
        fprintf(stderr, "%s: error: FreeBitmapWag: %s.\n", APP_NAME, 
            ErrorsToStringBitmapWag(error));
        return -1;
    }

    return 0; 
}

