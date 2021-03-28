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
#include <stdlib.h>
#include "BitmapWag.h"

/*
 * BitmapWagState indicates the state of the bitmap struct, so that 
 * initializations cannot occur twice so that the library prevents memory 
 * leaks.
 */ 
typedef enum {
    // No initialization has occured with this struct
    BITMAPWAG_STATE_NONE,
    // ConstructBitmapWag has been called
    BITMAPWAG_STATE_CONSTRUCTED,
    // InitializeBitmapWag or ReadBitmapWagcalled 
    BITMAPWAG_STATE_INITIALIZED
} BitmapWagState;

// Bitmap file header
typedef struct __attribute__((__packed__)) {
    // Always set to 'BM' to declare that this is a .bmp file
    uint16_t bfType; 
    // Size of file in bytes 
    uint32_t bfSize;  
    // always set to zero
    uint16_t bfReserved1; 
    // always set to zero
    uint16_t bfReserved2; 
    // specifies the offset from the beginning of the file to the bitmap data
    uint32_t bfOffBits;  
} BitmapWagBmfh;

// Bitmap info header 
typedef struct __attribute__((__packed__)) {
    // biSize specifies the size of the BITMAPINFORHEADER structure in btyes
    uint32_t biSize; 
    // biWidth specifies the width of the image in pixels
    uint32_t biWidth;
    // biHeight specifies the height of the image in pixels
    uint32_t biHeight;
    // specifies the number of planes of the target device, set to zero
    uint16_t biPlanes;
    // biBitCount specifies the number of bits per pixel, actually used to 
    // specify the color resolution of the bitmap:
    // 1: black/white
    // 4: 16-colors
    // 8: 256-colors
    // 24: 16.7 million colors
    uint16_t biBitCount;
    // biCompression specifies the type of compression, set to zero
    uint32_t biCompression;
    // biSizeImage specifies the size of the image data in bytes, if no 
    // compresion, set to zero
    uint32_t biSizeImage;
    // biXPelsPerMeter specifies the horizontal pixels per meter on the target 
    // device
    uint32_t biXPelsPerMeter;
    // biYPelsPerMeter specifies the vertical pixels per meter on the target 
    // device
    uint32_t biYPelsPerMeter;
    // biClrUsed specifies the number of colors used in the bitmap, if zero, 
    // num colors is calculated using biBitCount
    uint32_t biClrUsed;
    // biClrImportant specifies the number of colors that are important for the 
    // bitmap, if set to zero all colors are important. 
    uint32_t biClrImportant;
} BitmapWagBmih;

// Struct containing all the Bitmap structures 
struct BitmapWagImg {
    // Bitmap file header
    BitmapWagBmfh bmfh;
    // Bitmap info header 
    BitmapWagBmih bmih;
    // Color 'palette'
    BitmapWagRgbQuad * aColors;
    // image bits
    uint8_t * aBitmapBits;
    // colorUsed will be used by the bitmap array to keep track of how many 
    // colors in the pallet are being used when writing to pixels for 
    // efficiencies sake. 
    uint8_t * colorUsed;
    // state indicates the state of the bitmap struct, so that initializations
    // cannot occur twice so that the library prevents memory leaks. 
    BitmapWagState state;
}; 

//
const unsigned int MajorVersionBitmapWag(void)
{
    return 1;
}

const unsigned int MinorVersionBitmapWag(void)
{
    return 1;
}

const unsigned int PatchVersionBitmapWag(void)
{
    return 0;
}

/**
 * ceilLog2b16_t finds the log base 2 of a 16 bit integer and if there is a 
 * decimal, rounds up to the nearest integer. 
 * This is used internally by the libBitmapWag library. 
 *
 * @param input the value to take the log of
 * @return ceil(log2(input)) 
 */
static uint16_t ceilLog2b16_t (const uint16_t input)
{
    uint16_t highestBitPosition = 0;
    uint16_t numOneBits = 0;
    const unsigned sizeOfType = sizeof(uint16_t) << 3;
    for(unsigned i = 0; i < sizeOfType; i++)
    {
        unsigned currentBit = ((input >> i) & 0x1);
        if(currentBit > 0)
        {
            highestBitPosition = i;
        }
        numOneBits += currentBit; 
    }

    // Return is the sum of the position of the highest 1 bit
    // 1 if there are more than one 1 bits
    // 1 if the input is not zero
    return highestBitPosition + 
        ((numOneBits - 1) > 0 && (numOneBits - 1) < sizeOfType)
        + (input > 0);
}

/**
 * Find the amount of memory that needs to be allocated for the image array
 * This is used internally by the libBitmapWag library. 
 * 
 * @param width of image
 * @param bitsPerPixel of each pixel
 * @return the amount of memory that needs to be allocated for each row
 */
static uint32_t GetRowMemory(const uint32_t width, const uint16_t bitsPerPixel)
{
    uint32_t rowMemory = ((bitsPerPixel < 8) 
        * (width >> (4 - ceilLog2b16_t(bitsPerPixel)))) 
        + (width * (bitsPerPixel >> 3)); 

    const unsigned bytesAtEnd = 4;
    // Make sure there's enough space for each row to end on a four byte bound
    rowMemory += ((rowMemory % bytesAtEnd) != 0) * 
        (bytesAtEnd - (rowMemory % bytesAtEnd));

    return rowMemory;
}

/**
 * SetColorUsedArrayBitmapWag populates the 256 bit array colorUsed. 
 * This speeds up the function SetBitmapWagPixel when color palettes are used 
 * because the 256 byte array would need to be reconstructed each call 
 * otherwise. 
 * This is used internally by the libBitmapWag library. 
 *
 * @param bm pointer to a bitmap struct
 * @param colorUsed pointer to a 256 byte array for keeping track of which 
 *        colors in the color palette have been used. 
 * @return BITMAPWAG_SUCCESS if successful
 */
static BitmapWagError SetColorUsedArrayBitmapWag(BitmapWagImg * bm, 
    uint8_t * colorUsed)
{
    if(bm == NULL)
    {
        return BITMAPWAG_NULL;
    }

    size_t bitsPerPixel = bm->bmih.biBitCount;
    size_t width = bm->bmih.biWidth;
    size_t height = bm->bmih.biHeight;

    // Find the amount of memory that needs to be allocated for the image array
    size_t rowMemory = GetRowMemory(width, bitsPerPixel);

    
    if(colorUsed == NULL)
    {
        return BITMAPWAG_COLOR_ARRAY_NULL;
    }
    // Check if the bitmap bits pointer is null
    if(bm->aBitmapBits == NULL)
    {
        return BITMAPWAG_BITMAPBITS_NULL;
    }

    // If a color palette is not being used 
    if(bm->bmih.biBitCount > 8)
    {
        return BITMAPWAG_NO_COLOR_PALETTE; 
    }

    // Set the index, each pixel contains, to one in the colorUsed array 
    for(uint32_t i = 0; i < width; i++)
    {
        for(uint32_t j = 0; j < height; j++)
        {
            uint8_t value = 
            (bm->aBitmapBits)
            [j*rowMemory + (i >> (4 - ceilLog2b16_t(bitsPerPixel)))];

            value = (((0xFF >> (8 - bitsPerPixel)) 
                << (4 - ceilLog2b16_t(bitsPerPixel))) & value);

            value = value >> (4 - ceilLog2b16_t(bitsPerPixel));

            colorUsed[value] = 1;
        }
    }

    return BITMAPWAG_SUCCESS;
}

const char * ErrorsToStringBitmapWag(const BitmapWagError error)
{
    switch(error)
    {
        case BITMAPWAG_SUCCESS: 
            return "success";
        case BITMAPWAG_NULL: 
            return "bitmap null";
        case BITMAPWAG_FILE_PATH_NULL: 
            return "bitmap file null";
        case BITMAPWAG_CANNOT_OPEN_FILE: 
            return "bitmap cannot open file";
        case BITMAPWAG_ALLOCATE_BITMAP_BITS_FAILED: 
            return "bitmap allocate bitmap bits failed";
        case BITMAPWAG_ALLOCATE_PALETTE_FAILED: 
            return "bitmap allocate paletter failed";
        case BITMAPWAG_BMFH_NOT_WRITTEN:
            return "bitmap BMFH not written";
        case BITMAPWAG_BMIH_NOT_WRITTEN:
            return "bitmap BMIH not written";
        case BITMAPWAG_PALETTE_NOT_WRITTEN:
            return "bitmap palette not written because there's no space left";
        case BITMAPWAG_IMAGE_NOT_WRITTEN:
            return "bitmap image portion not written";
        case BITMAPWAG_COLOR_ARRAY_NULL:
            return "bitmap color used array null";
        case BITMAPWAG_NO_COLOR_PALETTE:
            return "bitmap no color palette used in this image";
        case BITMAPWAG_BIBITS_NOT_SUPPORTED:
            return "bitmap the bits per pixel value is not supported";
        case BITMAPWAG_COLOR_PTR_NULL:
            return "bitmap color pointer null";
        case BITMAPWAG_BITMAPBITS_NULL:
            return "bitmap aBitmapBits null";
        case BITMAPWAG_COLOR_PALETTE_NULL:
            return "bitmap aColor null";
        case BITMAPWAG_COORDINATE_WIDTH_OUT:
            return "bitmap coordinate width out of bounds";
        case BITMAPWAG_COORDINATE_HEIGHT_OUT:
            return "bitmap coordinate height out of bounds";
        case BITMAPWAG_BMFH_NOT_READ:
            return "bitmap BMFH not read";
        case BITMAPWAG_BMIH_NOT_READ:
            return "bitmap BMIH not read";
        case BITMAPWAG_ACOLORS_NOT_READ:
            return "bitmap color palette portion of file not read";
        case BITMAPWAG_BITMAPBITS_NOT_READ:
            return "bitmap image portion of file not read";
        case BITMAPWAG_COLORUSED_FAILED_TO_ALLOCATE:
            return "failed to allocate internal colorUsed array, perfomance \
will be degraded";
        case BITMAPWAG_NOTCONSTRUCTED: 
            return "bitmap not constructed, ConstructBitmapWag() needs to be \
needs to be called";
        case BITMAPWAG_ALREADY_INIT:
            return "bitmap has already been initialized and would be \
initialized twice by calling this function";
        default: 
            return "unknown error"; 
    }
}

BitmapWagImg * ConstructBitmapWag(void) 
{
    BitmapWagImg * output = (BitmapWagImg *) malloc(sizeof(BitmapWagImg));
    if(output != NULL)
    {
        *output = (BitmapWagImg){0};
        output->state = BITMAPWAG_STATE_CONSTRUCTED;
    }
    return output;
}

BitmapWagError ReadBitmapWag(BitmapWagImg * bm, const char * filePath)
{
    FILE *fp;
    size_t itemsRead;
    BitmapWagError retVal = BITMAPWAG_SUCCESS;

    if(bm == NULL)
    {
        return BITMAPWAG_NULL;
    }

    if (filePath == NULL)
    {
        return BITMAPWAG_FILE_PATH_NULL;
    }

    // Check to make sure that the object hasn't already been initialized
    if(bm->state == BITMAPWAG_STATE_NONE)
    {
        return BITMAPWAG_NOTCONSTRUCTED;
    }
    else if(bm->state == BITMAPWAG_STATE_INITIALIZED)
    {
        return BITMAPWAG_ALREADY_INIT;
    }

    bm->bmih = (BitmapWagBmih){0};
    bm->bmfh = (BitmapWagBmfh){0};
    bm->aColors = NULL;
    bm->aBitmapBits = NULL;
    bm->colorUsed = NULL;

    // open filePath as binary file for writing 
    fp = fopen(filePath, "rb");

    if(fp == NULL)
    {
        return BITMAPWAG_CANNOT_OPEN_FILE;
    }

    // Read the bitmap file header 
    itemsRead = fread(&(bm->bmfh), sizeof(bm->bmfh), 1, fp);

    if(itemsRead != 1)
    {
        fclose(fp);
        return BITMAPWAG_BMFH_NOT_READ;
    }

    // Read the bitmap info header 
    itemsRead = fread(&(bm->bmih), sizeof(bm->bmih), 1, fp);

    if(itemsRead != 1)
    {
        fclose(fp);
        return BITMAPWAG_BMIH_NOT_READ;
    }
    
    uint16_t bitsPerPixel = bm->bmih.biBitCount;
    uint32_t width = bm->bmih.biWidth;
    uint32_t height = bm->bmih.biHeight;

    // Find the amount of memory that needs to be allocated for the image array
    size_t rowMemory = GetRowMemory(width, bitsPerPixel);

    // fseek ahead if the bmih was larger than this library anticipated
    if(bm->bmih.biSize > sizeof(bm->bmih))
    {
        fseek(fp, bm->bmih.biSize - sizeof(bm->bmih), SEEK_CUR);
    }

    // Read the color palette if we're using 256-colors or less
    if(bm->bmih.biBitCount <= 8)
    {
        size_t sizeOfPalette;
        size_t sizeOfColorUsed;
        size_t numColors;
        if(bm->bmih.biClrUsed > 0)
        {
            numColors = bm->bmih.biClrUsed;
        }
        else
        {
            // Calculate number of colors based on biBitCount
            numColors = 1 << bm->bmih.biBitCount;    
        }
        // Calculate size of arrays to allocate 
        sizeOfPalette = numColors * sizeof(BitmapWagRgbQuad);
        sizeOfColorUsed = numColors * sizeof(uint8_t);

        bm->aColors = (BitmapWagRgbQuad *) malloc(sizeOfPalette);

        if(bm->aColors == NULL)
        {
            fclose(fp);
            return BITMAPWAG_ALLOCATE_PALETTE_FAILED;
        }
        // Indicate that bm has been initialized
        bm->state = BITMAPWAG_STATE_INITIALIZED;

        itemsRead = fread(bm->aColors, sizeOfPalette, 1, fp);

        if(itemsRead != 1)
        {
            fclose(fp);
            return BITMAPWAG_ACOLORS_NOT_READ;
        }

        // Allocate the space for the colorUsed record
        bm->colorUsed = (uint8_t *) malloc(sizeOfColorUsed);

        // if colorUsed failed to allocate, it's not an error, and there are 
        // fallbacks but it will really  slow down the performance of the 
        // library so the application should be notified 
        if(bm->colorUsed == NULL)
        {
            retVal = BITMAPWAG_COLORUSED_FAILED_TO_ALLOCATE;
        }
        else
        {
            // Initialize all values in colorUsed to be all zeros
            for(size_t i = 0; i < numColors; i++)
            {
                (bm->colorUsed)[i] = 0;
            }
        }
    }
    else
    {
        bm->aColors = NULL;
    }

    size_t bytesForImage = rowMemory * height;

    // Allocate the memory for the image
    bm->aBitmapBits = (uint8_t *) malloc(bytesForImage);

    if(bm->aBitmapBits == NULL)
    {
        fclose(fp);
        return BITMAPWAG_ALLOCATE_BITMAP_BITS_FAILED;
    }

    // Indicate that bm has been initialized
    bm->state = BITMAPWAG_STATE_INITIALIZED;

    // Read the image into memory 
    itemsRead = fread(bm->aBitmapBits, bytesForImage, 1, fp);

    if(itemsRead != 1)
    {
        fclose(fp);
        return BITMAPWAG_BITMAPBITS_NOT_READ;
    }

    //close the file
    fclose(fp);

    // Initialize color used array
    if(bm->bmih.biBitCount <= 8 && bm->colorUsed != NULL)
    {
        SetColorUsedArrayBitmapWag(bm, bm->colorUsed);
        // NOTE: We don't look at error values because none of them are valid
        // for this function in this usage. 
    }

    // Return successful 
    return retVal;
}

BitmapWagError WriteBitmapWag(const BitmapWagImg * bm, 
    const char * filePath)
{
    // Pointer to the file
    FILE *fp;
    size_t itemsWritten;

    // Check for null pointers before anything is done in this function.
    if(bm == NULL) 
    {
        return BITMAPWAG_NULL;
    }
    if(bm->state != BITMAPWAG_STATE_INITIALIZED)
    {
        return BITMAPWAG_NOT_INIT;
    }
    if (filePath == NULL)
    {
        return BITMAPWAG_FILE_PATH_NULL;
    }
    // Check to on the bitmap bits pointer
    if(bm->aBitmapBits == NULL)
    {
        return BITMAPWAG_BITMAPBITS_NULL;
    }

    // open filePath as binary file for writing 
    fp = fopen(filePath, "wb");

    if(fp == NULL)
    {
        return BITMAPWAG_CANNOT_OPEN_FILE;
    }

    // Write the bitmap file header 
    itemsWritten = fwrite(&(bm->bmfh), sizeof(bm->bmfh), 1, fp);

    if(itemsWritten != 1)
    {
        fclose(fp);
        return BITMAPWAG_BMFH_NOT_WRITTEN;
    }

    // Write the bitmap info header 
    itemsWritten = fwrite(&(bm->bmih), sizeof(bm->bmih), 1, fp);

    if(itemsWritten != 1)
    {
        fclose(fp);
        return BITMAPWAG_BMIH_NOT_WRITTEN;
    }
    
    uint16_t bitsPerPixel = bm->bmih.biBitCount;
    uint32_t width = bm->bmih.biWidth;
    uint32_t height = bm->bmih.biHeight;

    // Find the amount of memory that needs to be allocated for the image array
    size_t rowMemory = GetRowMemory(width, bitsPerPixel);
    
    size_t bytesForImage = rowMemory * height;

    // Write the color palette if we're using 256-colors or less
    if(bm->bmih.biBitCount <= 8)
    {
        if(bm->aColors == NULL)
        {
            fclose(fp);
            return BITMAPWAG_COLOR_PALETTE_NULL;
        }

        size_t sizeOfPalette;
        if(bm->bmih.biClrUsed > 0)
        {
            sizeOfPalette = bm->bmih.biClrUsed * sizeof(BitmapWagRgbQuad);
        }
        else
        {
            // Calculate number of colors based on biBitCount
            size_t numColors = 1 << bm->bmih.biBitCount;
            sizeOfPalette = numColors * sizeof(BitmapWagRgbQuad);
        }

        itemsWritten = fwrite(bm->aColors, sizeof(BitmapWagRgbQuad), 
            sizeOfPalette/sizeof(BitmapWagRgbQuad), fp);

        if(itemsWritten != sizeOfPalette/sizeof(BitmapWagRgbQuad))
        {
            fclose(fp);
            return BITMAPWAG_PALETTE_NOT_WRITTEN;
        }
    }

    // Write the aBitmapBits array
    itemsWritten = fwrite(bm->aBitmapBits, sizeof(uint8_t), 
        bytesForImage/sizeof(uint8_t), fp);

    if(itemsWritten != bytesForImage/sizeof(uint8_t))
    {
        fclose(fp);
        return BITMAPWAG_IMAGE_NOT_WRITTEN;
    }
    
    //close the file
    fclose(fp);

    // Return successful 
    return BITMAPWAG_SUCCESS;
}

BitmapWagError InitializeBitmapWag(BitmapWagImg * bm, const uint32_t height, 
    const uint32_t width, const uint16_t bitsPerPixel)
{
    // Contains the size of the palette array 
    size_t sizeOfPalette = 0; 
    BitmapWagError retVal = BITMAPWAG_SUCCESS;

    // Null check on bitmap pointer
    if(bm == NULL)
    {
        return BITMAPWAG_NULL;
    }

    // Check to make sure that the object hasn't already been initialized
    if(bm->state == BITMAPWAG_STATE_NONE)
    {
        return BITMAPWAG_NOSTATE;
    }
    else if(bm->state == BITMAPWAG_STATE_INITIALIZED)
    {
        return BITMAPWAG_ALREADY_INIT;
    }

    bm->colorUsed = NULL;

    // Find the amount of memory that needs to be allocated for the image array
    size_t rowMemory = GetRowMemory(width, bitsPerPixel);
    
    size_t bytesForImage = rowMemory * height;

    // Allocate the memory for the image
    bm->aBitmapBits = (uint8_t *) malloc(bytesForImage);

    if(bm->aBitmapBits == NULL)
    {
        return BITMAPWAG_ALLOCATE_BITMAP_BITS_FAILED;
    }

    // Indicate that the bitmap has been initialized 
    bm->state = BITMAPWAG_STATE_INITIALIZED;

    // Make the entire color array point to the zeroth color palette index or 
    // make it colored black 
    for(size_t i = 0; i < bytesForImage; i++)
    {
        (bm->aBitmapBits)[i] = 0x00;
    }

    // Allocate memory for the color palette if one is needed 
    // And allocate memory for the colorsUsed array if one is needed
    if(bitsPerPixel <= 8)
    {
        // Calculate number of colors based on biBitCount
        size_t numColors = 1 << bitsPerPixel;
        size_t sizeOfColorUsed = numColors * sizeof(uint8_t);
        sizeOfPalette = numColors * sizeof(BitmapWagRgbQuad);

        bm->aColors = (BitmapWagRgbQuad *) malloc(sizeOfPalette);

        if(bm->aColors == NULL)
        {
            return BITMAPWAG_ALLOCATE_PALETTE_FAILED;
        }

        bm->bmih.biClrUsed = numColors;

        // Make the entire color palette black
        for(size_t i = 0; i < numColors; i++)
        {
            (bm->aColors)[i] = (BitmapWagRgbQuad){0x00, 0x00, 0x00, 0};
        }

        // Allocate the space for the colorUsed record
        bm->colorUsed = (uint8_t *) malloc(sizeOfColorUsed);

        // if colorUsed failed to allocate, it's not an error, and there are 
        // fallbacks but it will really  slow down the performance of the 
        // library so the application should be notified 
        if(bm->colorUsed == NULL)
        {
            retVal = BITMAPWAG_COLORUSED_FAILED_TO_ALLOCATE;
        }
        else
        {
            // Initialize all values in colorUsed to be all zeros
            for(size_t i = 0; i < numColors; i++)
            {
                (bm->colorUsed)[i] = 0;
            }
        }
    }
    else
    {
        bm->bmih.biClrUsed = 0;
        bm->aColors = NULL;
        bm->colorUsed = NULL;
    }

    // Now that everything has been allocated, start initializing all the 
    // member variables
    
    // Set bmfh
    ((char *)&(bm->bmfh.bfType))[0] = 'B';
    ((char *)&(bm->bmfh.bfType))[1] = 'M';

    bm->bmfh.bfSize = sizeof(bm->bmih) + sizeof(bm->bmfh) + sizeOfPalette 
        + bytesForImage;

    bm->bmfh.bfReserved1 = 0;
    bm->bmfh.bfReserved2 = 0;
    bm->bmfh.bfOffBits = sizeof(bm->bmfh) + sizeof(bm->bmih) + sizeOfPalette;

    // Set bmih
    bm->bmih.biSize = sizeof(bm->bmih);
    bm->bmih.biWidth = width;
    bm->bmih.biHeight = height;
    bm->bmih.biPlanes = 1;

    bm->bmih.biBitCount = bitsPerPixel;
    bm->bmih.biCompression = 0;
    bm->bmih.biSizeImage = 0;
    // 72 DPI
    bm->bmih.biXPelsPerMeter = 28346;
    bm->bmih.biYPelsPerMeter = 28346;

    bm->bmih.biClrImportant = 0;

    return retVal;
}

BitmapWagError FreeBitmapWag(BitmapWagImg * bm)
{
    // Null check on bitmap pointer
    if(bm == NULL)
    {
        return BITMAPWAG_NULL;
    }
    
    if(bm->state == BITMAPWAG_STATE_INITIALIZED)
    {
        if(bm->aBitmapBits != NULL)
        {
            free(bm->aBitmapBits);
            bm->aBitmapBits = NULL;
        }

        if(bm->aColors != NULL)
        {
            free(bm->aColors);
            bm->aColors = NULL;
        }

        if(bm->colorUsed != NULL)
        {
            free(bm->colorUsed);
            bm->colorUsed = NULL;
        }
    }
    
    if(bm->state == BITMAPWAG_STATE_INITIALIZED || 
       bm->state == BITMAPWAG_STATE_CONSTRUCTED)
    {
        free(bm);
        bm = NULL;
    }
    else
    {
        return BITMAPWAG_NOSTATE;
    }

    return BITMAPWAG_SUCCESS; 
}

uint32_t GetBitmapWagHeight(const BitmapWagImg * bm)
{
    // Null check on bitmap pointer
    if(bm == NULL)
    {   
        return 0;
    }
    else
    {
        return bm->bmih.biHeight;
    }
}

uint32_t GetBitmapWagWidth(const BitmapWagImg * bm)
{
    // Null check on bitmap pointer
    if(bm == NULL)
    {   
        return 0;
    }
    else
    {
        return bm->bmih.biWidth;
    }
}

/**
 * compares color a to color b for equivalence
 *
 * @param a first color to compare
 * @param b second color to compare
 * @return 0 if identical
 */
static uint32_t CompareColors(const BitmapWagRgbQuad a, 
    const BitmapWagRgbQuad b)
{
    return (a.rgbBlue == b.rgbBlue) && (a.rgbGreen == b.rgbGreen) 
        && (a.rgbRed == b.rgbRed) && (a.rgbReserved == b.rgbReserved);
}

BitmapWagError SetBitmapWagPixel(BitmapWagImg * bm, const uint32_t x, 
    const uint32_t y, const uint8_t r, const uint8_t g, const uint8_t b)
{
    // colorUsedInt keeps track of which indicies in a color palette are in use
    // Use a stack allocated array because it's relatively small and it's 
    // much faster than dynamic allocations 
    uint8_t colorUsedInt [256] = {0};
    uint16_t possibleColors;

    // Null check on bitmap pointer
    if(bm == NULL)
    {
        return BITMAPWAG_NULL;
    }

    // Check to make sure that the object has already been initialized
    if(bm->state != BITMAPWAG_STATE_INITIALIZED)
    {
        return BITMAPWAG_NOT_INIT;
    }

    // Check to on the bitmap bits pointer
    if(bm->aBitmapBits == NULL)
    {
        return BITMAPWAG_BITMAPBITS_NULL;
    }

    const size_t bitsPerPixel = bm->bmih.biBitCount;
    const uint32_t width = bm->bmih.biWidth;
    const uint32_t height = bm->bmih.biHeight;

    if(x >= width)
    {
        return BITMAPWAG_COORDINATE_WIDTH_OUT;
    }

    if(y >= height)
    {
        return BITMAPWAG_COORDINATE_HEIGHT_OUT;
    }

    // Find the amount of memory that needs to be allocated for the image array
    const size_t rowMemory = GetRowMemory(width, bitsPerPixel);

    // If a color palette is being used 
    if(bm->bmih.biBitCount <= 8)
    {
        BitmapWagRgbQuad color = {b, g, r, 0};
        uint8_t colorNotFound = 1;
        uint8_t indexOfColor; 

        if(bm->aColors == NULL)
        {
            return BITMAPWAG_COLOR_PALETTE_NULL;
        }

        // Get the value at the byte
        uint8_t value = (bm->aBitmapBits)
            [y*rowMemory + (x >> (4 - ceilLog2b16_t(bitsPerPixel)))];

        if(bm->bmih.biClrUsed > 0)
        {
            possibleColors = bm->bmih.biClrUsed;
        }
        else
        {
            possibleColors = 1 << bm->bmih.biBitCount;
        }

        // Populate the contents of the colorUsed array

        uint8_t * colorUsedPtr;
        if(bm->colorUsed == NULL)
        {
            colorUsedPtr = colorUsedInt;
            SetColorUsedArrayBitmapWag(bm, colorUsedPtr);
        }
        else
        {
            colorUsedPtr = bm->colorUsed;
        }

        // Find the index of the color specified in the input
        for(uint16_t i = 0; i < possibleColors; i++)
        { 
            if(CompareColors((bm->aColors)[i], color) && colorUsedPtr[i])
            {
                // If the colors are the same and the color is being used
                // then set the index
                colorNotFound = 0;
                indexOfColor = i;
                break;
            }
        }

        // If the color is not yet in the palette, find a new place for it
        if(colorNotFound)
        {
            // Ensure colorNotFound is 1 because now it will be used to indicate
            // if there is no space left in the color palette 
            colorNotFound = 1;

            // Find the index of the color specified in the input
            for(uint16_t i = 0; i < possibleColors; i++)
            {
                if(!colorUsedPtr[i])
                {
                    colorUsedPtr[i] = 1;
                    colorNotFound = 0;
                    (bm->aColors)[i] = color;
                    indexOfColor = i;
                    break;
                }
            }

            // If there was no space left in the palette 
            if(colorNotFound)
            {
                return BITMAPWAG_PALETTE_NOT_WRITTEN;
            }
        }

        // Amount to shift palette index by
        uint8_t sftAmnt = (bitsPerPixel * 
            ((~x) & (~(0xFFFFFFFF << (4 - ceilLog2b16_t(bitsPerPixel))))));

        // Mask holds the right shifted bit mask
        uint8_t mask = (0xFF >> (8 - bitsPerPixel));

        // First iteration sets the index of color to the correct bit width
        uint8_t value2 = (indexOfColor & (0xFF >> (8 - bitsPerPixel)));
        // Second iteration shifts palette index to the correct byte position
        value2 = value2 << sftAmnt;
        // Third iteration masks out value for value2 to be inserted in 
        value = value & (~(mask << sftAmnt));
        // Fourth iteration combines the values
        value = value | value2;

        (bm->aBitmapBits)
            [y*rowMemory + (x >> (4 - ceilLog2b16_t(bitsPerPixel)))] = value;
    }

    // biBitCount will either be 16 or 24 when a color palette is not being used
    else if (bm->bmih.biBitCount == 16)
    {
        uint16_t * bitmap16 = (uint16_t *) bm->aBitmapBits;

        bitmap16[y*rowMemory/2 + x] = 0 | ((0x1F & b) << 10) 
            | ((0x1F & g) << 5) | ((0x1F & r) << 0);
    }

    else if(bm->bmih.biBitCount == 24)
    {
        (bm->aBitmapBits)[y*rowMemory + 3*x] = b;
        (bm->aBitmapBits)[y*rowMemory + 3*x + 1] = g;
        (bm->aBitmapBits)[y*rowMemory + 3*x + 2] = r;
    }

    else if(bm->bmih.biBitCount == 32)
    {
        (bm->aBitmapBits)[y*rowMemory + 4*x] = b;
        (bm->aBitmapBits)[y*rowMemory + 4*x + 1] = g;
        (bm->aBitmapBits)[y*rowMemory + 4*x + 2] = r;
        (bm->aBitmapBits)[y*rowMemory + 4*x + 3] = 0;
    }

    else
    {
        //TODO: Remove
        printf("bm->bmih.biBitCount %d\n", bm->bmih.biBitCount);
        return BITMAPWAG_BIBITS_NOT_SUPPORTED;
    }
        

    return BITMAPWAG_SUCCESS;
}


BitmapWagError GetBitmapWagPixel(const BitmapWagImg * bm, 
    const uint32_t x, const uint32_t y, BitmapWagRgbQuad * color)
{
    // Null check on bitmap pointer
    if(bm == NULL)
    {
        return BITMAPWAG_NULL;
    }

    if(color == NULL)
    {
        return BITMAPWAG_COLOR_PALETTE_NULL;
    }

    // Check to make sure that the object has already been initialized
    if(bm->state != BITMAPWAG_STATE_INITIALIZED)
    {
        return BITMAPWAG_NOT_INIT;
    }

    const size_t bitsPerPixel = bm->bmih.biBitCount;
    const uint32_t width = bm->bmih.biWidth;
    const uint32_t height = bm->bmih.biHeight;

    if(x >= width)
    {
        return BITMAPWAG_COORDINATE_WIDTH_OUT;
    }

    if(y >= height)
    {
        return BITMAPWAG_COORDINATE_HEIGHT_OUT;
    }

    // Find the amount of memory that needs to be allocated for the image array
    const size_t rowMemory = GetRowMemory(width, bitsPerPixel);

    // If a color palette is being used 
    if(bm->bmih.biBitCount <= 8)
    {
        if(bm->aColors == NULL)
        {
            return BITMAPWAG_COLOR_PALETTE_NULL;
        }

        // Get the value at the byte
        uint8_t value = (bm->aBitmapBits)
            [y*rowMemory + (x >> (4 - ceilLog2b16_t(bitsPerPixel)))];

        // Amount to shift palette index by
        uint8_t sftAmnt = (bitsPerPixel * 
            ((~x) & (~(0xFFFFFFFF << (4 - ceilLog2b16_t(bitsPerPixel))))));

        // Mask holds the right shifted bit masks
        uint8_t mask = (0xFF >> (8 - bitsPerPixel));

        // Snipe out the bit(s) of the pixel that are used to look up a color 
        // in the palette 
        value = (value >> sftAmnt) & mask;

        color->rgbBlue = (bm->aColors)[value].rgbBlue;
        color->rgbGreen = (bm->aColors)[value].rgbGreen;
        color->rgbRed = (bm->aColors)[value].rgbRed;
        color->rgbReserved = (bm->aColors)[value].rgbReserved;
    }

    // biBitCount will either be 16 or 24 when a color palette is not being used
    else if (bm->bmih.biBitCount == 16)
    {
        uint16_t * bitmap16 = (uint16_t *) bm->aBitmapBits;

        color->rgbBlue = (bitmap16[y*rowMemory/2 + x] >> 10) & 0x001F;
        color->rgbGreen = (bitmap16[y*rowMemory/2 + x] >> 5) & 0x001F;
        color->rgbRed = (bitmap16[y*rowMemory/2 + x] >> 0) & 0x001F;
        color->rgbReserved = 0;
    }

    else if(bm->bmih.biBitCount == 24)
    {
        color->rgbBlue = (bm->aBitmapBits)[y*rowMemory + 3*x];
        color->rgbGreen = (bm->aBitmapBits)[y*rowMemory + 3*x + 1];
        color->rgbRed = (bm->aBitmapBits)[y*rowMemory + 3*x + 2];
        color->rgbReserved = 0;
    }
    else if(bm->bmih.biBitCount == 32)
    {
        color->rgbBlue = (bm->aBitmapBits)[y*rowMemory + 4*x];
        color->rgbGreen = (bm->aBitmapBits)[y*rowMemory + 4*x + 1];
        color->rgbRed = (bm->aBitmapBits)[y*rowMemory + 4*x + 2];
        color->rgbReserved = (bm->aBitmapBits)[y*rowMemory + 4*x + 3];
    }
    else
    {
        return BITMAPWAG_BIBITS_NOT_SUPPORTED;
    }
    
    return BITMAPWAG_SUCCESS;
}

