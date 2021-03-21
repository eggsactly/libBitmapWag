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

#ifndef LIB_BITMAP_WAG
#define LIB_BITMAP_WAG

// Added to make library compatible with C and C++. 
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h> 

// Errors that can come from bitmap operations
typedef enum {
    BITMAPWAG_SUCCESS = 0,
    BITMAPWAG_NULL,
    BITMAPWAG_FILE_PATH_NULL,
    BITMAPWAG_CANNOT_OPEN_FILE,
    BITMAPWAG_ALLOCATE_BITMAP_BITS_FAILED,
    BITMAPWAG_ALLOCATE_PALETTE_FAILED,
    BITMAPWAG_BMFH_NOT_WRITTEN,
    BITMAPWAG_BMIH_NOT_WRITTEN,
    BITMAPWAG_PALETTE_NOT_WRITTEN,
    BITMAPWAG_IMAGE_NOT_WRITTEN,
    BITMAPWAG_OUT_OF_COLORS,
    BITMAPWAG_COLOR_ARRAY_NULL,
    BITMAPWAG_NO_COLOR_PALETTE,
    BITMAPWAG_BIBITS_NOT_SUPPORTED,
    BITMAPWAG_COLOR_PTR_NULL,
    BITMAPWAG_BITMAPBITS_NULL,
    BITMAPWAG_COLOR_PALETTE_NULL,
    BITMAPWAG_COORDINATE_WIDTH_OUT,
    BITMAPWAG_COORDINATE_HEIGHT_OUT,
    BITMAPWAG_BMFH_NOT_READ,
    BITMAPWAG_BMIH_NOT_READ,
    BITMAPWAG_ACOLORS_NOT_READ,
    BITMAPWAG_BITMAPBITS_NOT_READ,
    BITMAPWAG_COLORUSED_FAILED_TO_ALLOCATE,
    BITMAPWAG_PTRNULL
} BitmapWagError;

typedef struct BitmapWagImg BitmapWagImg;

// Red Green Blue quad struct
// Does not need to be packed because members are all of the same type
typedef struct {
    // specifies the blue part of the color 
    uint8_t rgbBlue; 
    // specifies the green part of the color
    uint8_t rgbGreen; 
    // specifies the red part of the color
    uint8_t rgbRed; 
    // must always be set to zero
    uint8_t rgbReserved; 
} BitmapWagRgbQuad; 

/**
 *  MajorVersionBitmapWag returns the major version number of the library
 *  This library uses symantic version numbering
 *  @return major version number of the library
 */
const unsigned int MajorVersionBitmapWag(void);

/**
 *  MinoVersionBitmapWag returns the minor version number of the library
 *  This library uses symantic version numbering
 *  @return minor version number of the library
 */
const unsigned int MinorVersionBitmapWag(void);

/**
 *  PatchVersionBitmapWag returns the patch version number of the library
 *  This library uses symantic version numbering
 *  @return patch version number of the library
 */
const unsigned int PatchVersionBitmapWag(void);

/** ErrorsToStringBitmapWag takes an error id and translates it to a human 
 *  readable string
 *  @param error code
 *  @return error string
 */ 
const char * ErrorsToStringBitmapWag(const BitmapWagError error);

/**
 * WriteBitmapWag writes a bitmap image file
 *
 * @param bm pointer to a Bitmap_img struct
 * @param filePath path to write the file to, relative or absolute.
 * @return BITMAPWAG_SUCCESS if successful  
 */
BitmapWagError WriteBitmapWag(BitmapWagImg ** const bm, const char * filePath);

/**
 * ReadBitmapWag reads a bitmap image file
 *
 * @param bm pointer to a Bitmap_img struct
 * @param filePath path to read a file from, relative or absolute.
 * @return BITMAPWAG_SUCCESS if successful  
 * @note InitializeBitmapWag should not be called before this and could cause
 *       memory leaks if it is. 
 */
BitmapWagError ReadBitmapWag(BitmapWagImg ** bm, const char * filePath);

/**
 * InitializeBitmapWag creates a bitmap file 
 *
 * @param bm pointer to bitmap to populate
 * @param height of image
 * @param width of image
 * @param number of bits per pixel
 * @return BITMAPWAG_SUCCESS if successful
 */
BitmapWagError InitializeBitmapWag(BitmapWagImg ** bm, const uint32_t height, 
    const uint32_t width, const uint16_t bitsPerPixel);

/**
 *  FreeBitmapWag frees memory of a bitmap
 *
 * @param bm bitmap pointer
 * @return BITMAPWAG_SUCCESS if successful
 */
BitmapWagError FreeBitmapWag(BitmapWagImg ** bm);

/**
 * GetBitmapHeightWag gets the height of the bitmap
 * 
 * @param bm bitmap image
 * @return height of bitmap image
 */
uint32_t GetBitmapWagHeight(BitmapWagImg ** const bm);

/**
 * GetBitmapWagWidth gets the height of the bitmap
 * 
 * @param bm bitmap image
 * @return width of bitmap image
 */
uint32_t GetBitmapWagWidth(BitmapWagImg ** const bm);

/**
 * SetBitmapWagPixel sets a pixel on the bitmap to the specified color
 *
 * @param bm pointer to the bitmap image
 * @param x horizontal coordinate (from left)
 * @param y vertical coordinate (from bottom)
 * @param r red component
 * @param g green component
 * @param b blue component 
 * @return BITMAPWAG_SUCCESS if successful
 */
BitmapWagError SetBitmapWagPixel(BitmapWagImg ** bm, const uint32_t x, 
    const uint32_t y, const uint8_t r, const uint8_t g, const uint8_t b);

/**
 * GetBitmapWagPixel gets the color value at a coordinate on the image
 *
 * @param bm pointer to a bitmap struct
 * @param x horizontal coordinate (from left)
 * @param y vertical coordinate (from bottom)
 * @param color pointer to the color value to populate
 * @return BITMAPWAG_SUCCESS if successful
 */
BitmapWagError GetBitmapWagPixel(BitmapWagImg ** const bm, 
    const uint32_t x, const uint32_t y, BitmapWagRgbQuad * color);

// Added to make library compatible with C and C++. 
#ifdef __cplusplus
}
#endif

#endif // LIB_BITMAP_WAG

