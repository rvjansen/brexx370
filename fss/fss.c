/*-----------------------------------------------------------------------------
|  Copyright (c) 2012-2013, Tommy Sprinkle (tommy@tommysprinkle.com)
|
|  All rights reserved. Redistribution and use in source and binary forms,
|  with or without modification, are permitted provided that the following
|  conditions are met:
|
|    * Redistributions of source code must retain the above copyright notice,
|      this list of conditions and the following disclaimer.
|    * Redistributions in binary form must reproduce the above copyright
|      notice, this list of conditions and the following disclaimer in the
|      documentation and/or other materials provided with the distribution.
|    * Neither the name of the author nor the names of its contributors may
|      be used toendorse or promote products derived from this software
|      without specific prior written permission.
|
|   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
|   "AS IS" AND ANYEXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
|   LIMITED TO, THE IMPLIED WARRANTIESOF MERCHANTABILITY AND FITNESS
|   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENTSHALL THE
|   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
|   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
|   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
|   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
|   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
|   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
|   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
|   OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------*/

//---------------------------------------------------------------
// FSS - Full Screen Services for TSO
//
// Tommy Sprinkle - tommy@tommysprinkle.com
// December, 2012
//
//---------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "jccdummy.h"
#include "bmem.h"
#include "rxtso.h"
#include "fss.h"
#include "ldefs.h"

// Field Definition
struct sFields
{
    char *name;            // Field Name or Null String for TXT fields
    int   bufaddr;         // Field location - offset into 3270 buffer
    int   attr;            // Attribute values
    int   length;          // Field Length
    char *data;            // Field Data
};

// Return Attribute Values
#define XH(attr) ((attr >> 16) & 0xFF)    // Return Extended Highlighting Attribute
#define XC(attr) ((attr >>  8) & 0xFF)    // Return Extended Color Attribute
#define BA(attr) (attr & 0xFF)            // Return Basic 3270 Attribute

#define MAXPOS12BIT 4095
#define MAXPOS14BIT 16383

#define is14BitAddr(altrows, altcols) ((altrows * altcols) > MAXPOS12BIT) ? TRUE : FALSE

// FSS Environment Values
static bool           isStatic;
static struct sFields fssStaticFields[1024];   // Array of static fields
static int            fssStaticFieldCnt;      // Count of fields defined in array

static struct sFields fssFields[1024];   // Array of fields
static int            fssFieldCnt;      // Count of fields defined in array
static int            fssAID;           // Last 3270 AID value returned
static int            fssCSR;           // Position of Cursor at last read
static int            fssCSRPOS;        // Buffer Position to place Cursor at next write
static int            fssPrimaryCols;   // Primary screen screen size cols
static int            fssPrimaryRows;   // Primary screen screen size rows
static int            fssAlternateCols; // Alternate screen screen size cols
static int            fssAlternateRows; // Alternate screen screen size rows

static unsigned int offset2address(unsigned int offset, unsigned int max_row, unsigned int max_col) {

    unsigned int address;

    unsigned char b0;
    unsigned char b1;

    //if (is14BitAddr(max_row, max_col)) {
    if (offset > MAXPOS12BIT) {
        if (offset > MAXPOS14BIT) {
            offset = MAXPOS14BIT;
        }

        address = offset & 0x3FFFU;
    } else {
        if (offset > MAXPOS12BIT) {
            offset = MAXPOS12BIT;
        }

        b0 = xlate3270((int) offset / 64);
        b1 = xlate3270((int) offset % 64);

        address = (((unsigned int)b0 << 8U) | (b1));
    }

    return address;
}

static unsigned int address2offset(unsigned int address) {

    unsigned char b0 = address >> 8U;
    unsigned char b1 = address;

    unsigned int position;

    if (b0 & 0xC0U) {
        position = ((address & 0x3FU) | ((address & 0x3F00U) >> 2U) );
    } else {
        position = ((unsigned int)b0 << 8U) | (unsigned int)b1;
    }

    return position;
}

static unsigned int position2offset(unsigned int row, unsigned int col, unsigned int max_col) {
    return ( ((row - 1) * max_col) + (col - 1) );
}

static void offset2position(unsigned int offset, unsigned int *row, unsigned int *col, unsigned int max_col) {
    *row = (offset / max_col) + 1;
    *col = (offset % max_col) + 1;
}

//----------------------------------------
// Find the field located at a "pos"
// offset.
// Returns the INDEX + 1 of the field
//----------------------------------------
static int findFieldPos(int pos)
{
    int ix;

    if(fssFieldCnt < 1)                     // If no fields
        return 0;

    // dynamic fields
    for(ix = 0; ix < fssFieldCnt; ix++)     // Loop through Field Array
        if(pos == fssFields[ix].bufaddr)     // Check for match
            return (ix + 1);                  // Return index + 1

    // static fields
    for(ix = 0; ix < fssStaticFieldCnt; ix++)     // Loop through Field Array
        if(pos == fssStaticFields[ix].bufaddr)     // Check for match
            return (ix + 1) * -1;                  // Return index + 1 as negative

    return 0;                               // No match found
}

//----------------------------------------
// Find a field by Field Name
// Returns the INDEX + 1 of the field
//
//----------------------------------------
static int findField(char *fldName)
{
    int  ix;

    if(fssFieldCnt < 1)                     // If no fields
        return 0;

    // dynamic fields
    for(ix=0; ix < fssFieldCnt; ix++) {     // Loop through Field Array
        if (fssFields[ix].name != NULL) {
            if(!strcmp(fldName,fssFields[ix].name)) {
                return (ix + 1);            // Return index + 1
            }
        }
    }

    // static fields
    for(ix=0; ix < fssStaticFieldCnt; ix++) {     // Loop through Field Array
        if (fssStaticFields[ix].name != NULL) {
            if(!strcmp(fldName,fssStaticFields[ix].name)) {
                return (ix + 1) * -1;            // Return index + 1 as negativ
            }
        }
    }

    return 0;                               // No match found
}

//----------------------------------------
//  Update Field Data
//
//
//----------------------------------------
static int updtFld(int pos, char *data, int len)
{
    int ix;
    struct sFields *fields;

    ix = findFieldPos(pos);                 // Locate Field by start position
    if(ix == 0)
        return (char *) 0;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;

    if(len  <= fields[ix].length)        // If data fits
    {
        memcpy(fields[ix].data, data, len);   // Copy data
        *(fields[ix].data+len) = '\0';        // Terminate string
    }
    else                                    // ELSE - truncate data
    {
        memcpy(fields[ix].data, data, fields[ix].length);  // Copy max amount we can
        *(fields[ix].data + fields[ix].length) = '\0';     // Terminate string
    }

    return 0;
}

//----------------------------------------
//  Translate non-printable characters
//  in a string to a "."
//
//----------------------------------------
static char * makePrint(char *str)
{
    char *p;

    p = str;
    while(*p)                              // Loop through string
    {
        if(!isprint(*p))                   // If not a printable character
            *p = '.';                      // Replace with "."
        p++;                               // Next char
    }
    return str;
}

//----------------------------------------
//  Check a string to see if it is all
//  Numeric
//
//----------------------------------------
int fssIsNumeric(char * data)
{
    size_t len;
    int i;

    len = strlen(data);                    // Get string length
    if(len < 1)                            // Empty string is NOT Numeric
        return 0;

    for(i=0; i<len; i++)                   // Check each character
        if(!isdigit( *(data+i) ))
            return 0;

    return 1;                              // All characters are numbers
}

//----------------------------------------
// Check a string to see if it is all
// Hex digits
//
//----------------------------------------
int fssIsHex(char * data)
{
    size_t len;
    int i;

    len = strlen(data);                    // Get string length
    if(len < 1)                            // Empty string is not HEX
        return 0;

    for(i=0; i<len; i++)                   // Check each character
        if(!isxdigit( *(data+i) ))
            return 0;

    return 1;
}

//----------------------------------------
//  Check to see if a string is all blanks
//
//
//----------------------------------------
int fssIsBlank(char * data)
{
    int i;

    i = 0;

    do
    {
        if (!(*(data+i)))                    // Empty string is blank
            return 1;

        if( *(data+i) != ' ')                // Check each character
            return 0;
        i++;
    } while(1);

    return 1;                              // String is blank
}

//----------------------------------------
// Trim trailing blanks from a string
//
//
//----------------------------------------
char * fssTrim(char * data)
{
    size_t len;

    len = strlen(data);                     // Get string length
    if(len == 0)
        return data;

    while(len)                              // Start at end of string
    {
        if(*(data+len-1) != ' ')             // Exit on first non-blank
            return data;
        *(data+len-1) = '\0';                // Remove trailing blank
        len--;
    }

    return data;                            // Return trimmed string
}

//----------------------------------------
//  Initialize FSS Environment
//
//
//----------------------------------------
int fssInit(void)
{
    RX_GTTERM_PARAMS_PTR paramsPtr;

    typedef struct tScreenSize {
        byte bRows;
        byte bCols;
    } PRIMARY_SCREEN_SIZE, ALTERNATE_SCREEN_SIZE;

    PRIMARY_SCREEN_SIZE primaryScreenSize;
    ALTERNATE_SCREEN_SIZE alternateScreenSize;

    fssFieldCnt         = 0;                       // Set Field Count to Zero
    fssStaticFieldCnt   = 0;                       // Set static Field Count to Zero
    fssCSRPOS           = 0;                       // Reset Cursor Position for next write
    fssPrimaryCols      = 0;
    fssPrimaryRows      = 0;
    fssAlternateCols    = 0;
    fssAlternateRows    = 0;

    paramsPtr = MALLOC(sizeof(RX_GTTERM_PARAMS),"GTTERM");

    paramsPtr->primadr   = (unsigned int *) &primaryScreenSize;
    paramsPtr->altadr    = (unsigned int *) &alternateScreenSize;
    *paramsPtr->altadr  |= 0x80000000;
    paramsPtr->attradr   = 0;
    paramsPtr->termidadr = 0;

    gtterm(paramsPtr);

    fssPrimaryCols      = primaryScreenSize.bCols;
    fssPrimaryRows      = primaryScreenSize.bRows;
    fssAlternateCols    = alternateScreenSize.bCols;
    fssAlternateRows    = alternateScreenSize.bRows;

    stfsmode(1);                            // Begin TSO Fullscreen Mode
    sttmpmd(1);

    FREE(paramsPtr);
    return 0;
}

//----------------------------------------
//  Initialize a new static  Screen
//  Old one get destroyed
//
//----------------------------------------
int fssStatic(void)
{
    int ix;

    for(ix=0; ix < fssStaticFieldCnt; ix++)       // Loop through Field Array
    {
        if(fssStaticFields[ix].name)
            free(fssStaticFields[ix].name);       // Free field name
        if(fssStaticFields[ix].data)
            free(fssStaticFields[ix].data);       // Free field data
    }

    fssStaticFieldCnt = 0;                        // Reset field count

    isStatic = TRUE;

    return 0;
}

//----------------------------------------
//  Destroy Current Screen
//  Begin a new empty screen
//
//----------------------------------------
int fssReset(void)
{
    int ix;

    for(ix=0; ix < fssFieldCnt; ix++)       // Loop through Field Array
    {
        if(fssFields[ix].name)
            free(fssFields[ix].name);         // Free field name
        if(fssFields[ix].data)
            free(fssFields[ix].data);         // Free field data
    }

    fssFieldCnt = 0;                        // Reset field count
    fssAID      = 0;                        // Reset last AID value
    fssCSR      = 0;                        // Reset last Cursor position
    fssCSRPOS   = 0;                        // Reset Cursor position

    return 0;
}

//----------------------------------------
// Terminate FSS Environment
//
//
//----------------------------------------
int fssTerm(void)
{
    fssStatic();
    fssReset();                             // Call Reset to free storage

    stlineno(1);                        // Exit TSO Full Screen Mode
    stfsmode(0);
    sttmpmd(0);

    return 0;
}

//----------------------------------------
// Return Last AID value
//
//
//----------------------------------------
int fssGetAID(void)
{
    return fssAID;
}

//----------------------------------------
// Return alternate screen width
//
//
//----------------------------------------
int fssGetAlternateScreenWidth(void)
{
    return fssAlternateCols;
}

//----------------------------------------
// Return alternate screen height
//
//
//----------------------------------------
int fssGetAlternateScreenHeight(void)
{
    return fssAlternateRows;
}

//----------------------------------------
// Translate an attribute value
//
//
//----------------------------------------
int fssAttr(int attr)
{
    return ((attr & 0xFFFF00) | xlate3270( attr & 0xFF));
}

//----------------------------------------
//  Define a Text Field
//     row  - Beginning Row position of field
//     col  - Beginning Col position of field
//     attr - Field Attribute
//     text - Field data contents
//
//----------------------------------------
int fssTxt(int row, int col, int attr, char * text)
{
    int txtlen;
    int ix;

    struct sFields *fields;
    int    *fieldCount;

    if (isStatic)
    {
        fields = fssStaticFields;
        fieldCount = &fssStaticFieldCnt;
    } else
    {
        fields = fssFields;
        fieldCount = &fssFieldCnt;
    }

    makePrint(text);                        // Eliminate non-printable characters
    txtlen = strlen(text);                  // get text length

    // Validate Field Starting Position
    if(row < 1 || col < 2 || row > fssAlternateRows || col > fssAlternateCols)
        return -1;

    if(txtlen < 1 || txtlen > (fssAlternateCols-1))           // Validate Maximum Length
        return -2;

    ix = findFieldPos((int) position2offset(row,col,fssAlternateCols));
    if (!ix) {
        (*(fieldCount))++;                  // Increment field count
        ix = *fieldCount;
    }
    ix--;                                   // count2index

    //----------------------------
    // Fill In Field Array Values
    //----------------------------
    fields[ix].name    =  0;             // no name for a text field
    fields[ix].bufaddr =  (int) position2offset(row,col,fssAlternateCols);
    fields[ix].attr    =  fssAttr(attr);
    fields[ix].length  =  txtlen;
    fields[ix].data    =  (char *) malloc(txtlen+1);
    strcpy(fields[ix].data, text);

    return 0;
}

//----------------------------------------
// Check presence of a field
//     fldName - Field name - to allow access
//
//----------------------------------------
int fssFieldExists(char *fldName)
{
    return (findField(fldName) == 0) ? (0) : (1);
}

//----------------------------------------
// Define a Dynamic Field
//     row     - Beginning Row position of field
//     col     - Beginning Col position of field
//     attr    - Field Attribute
//     fldName - Field name - to allow access
//     len     - Field length
//     text    - Field initial data contents
//
//----------------------------------------
int fssFld(int row, int col, int attr, char * fldName, int len, char *text)
{
    int ix;

    struct sFields *fields;
    int    *fieldCount;

    if (isStatic)
    {
        fields = fssStaticFields;
        fieldCount = &fssStaticFieldCnt;
    } else
    {
        fields = fssFields;
        fieldCount = &fssFieldCnt;
    }

    // Validate Field Start Position
    if(row < 1 || col < 2 || row > fssAlternateRows || col > fssAlternateCols)
        return -1;

    // Validate Field Length
    if(len < 1 || len > (fssAlternateCols-1))
        return -2;

    if(findField(fldName))                  // Check for duplicate Field Name
        //return -3;
        return 4;

    ix = findFieldPos((int) position2offset(row,col,fssAlternateCols));
    if (!ix) {
        (*(fieldCount))++;                  // Increment field count
        ix = *fieldCount;
    }
    ix--;

    //----------------------------
    // Fill In Field Array Values
    //----------------------------
    fields[ix].name    =  (char *) malloc(strlen(fldName)+1);
    strcpy(fields[ix].name, fldName);
    fields[ix].bufaddr =  (int)position2offset(row,col,fssAlternateCols);
    fields[ix].attr    =  fssAttr(attr);
    fields[ix].length  =  len;
    fields[ix].data    =  (char *) malloc(len + 1);

    makePrint(text);                        // Eliminate non-printable characters

    if(strlen(text) <= fields[ix].length)   // Copy text if it fits into field
        strcpy( fields[ix].data, text);
    else                                       // Truncate text if too long
    {
        strncpy(fields[ix].data, text, fields[ix].length);
        *(fields[ix].data + fields[ix].length) = '\0';
    }

    return 0;
}

//----------------------------------------
//  Set Dynamic Field Contents
//
//
//----------------------------------------
int fssSetField(char *fldName, char *text)
{
    int ix;

    struct sFields *fields;

    ix = findField(fldName);                // Locate Field by Name

    if(ix == 0)
        return -4;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;                                   // Actual Array Index Value

    makePrint(text);                        // Eliminate non-printable characters

    if(strlen(text) <= fields[ix].length)   // If text fits, copy it
        strcpy( fields[ix].data, text);
    else                                    // Truncate if too long
    {
        strncpy(fields[ix].data, text, fields[ix].length);
        *(fields[ix].data + fields[ix].length) = '\0';
    }

    return 0;
}

//----------------------------------------
//  Return pointer to Dynamic Field Contents
//
//
//----------------------------------------
char * fssGetField(char *fldName)
{
    int ix;
    struct sFields *fields;

    ix = findField(fldName);                // Find Field by Name
    if(ix == 0)
        return (char *) 0;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;

    return fields[ix].data;            // Return pointer to data
}

//----------------------------------------
//  Set Cursor position for next write
//
//
//----------------------------------------
int fssSetCursor(char *fldName)
{
    int ix;
    struct sFields *fields;

    ix = findField(fldName);                // Find Field by Name
    if(ix == 0)
        return (char *) 0;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;

    fssCSRPOS = (int) offset2address(fields[ix].bufaddr, fssAlternateRows, fssAlternateCols);   // Cursor pos = field start position

    return 0;
}

//----------------------------------------
// Replace Field Attribute Value
// This only replaces the 3270 basic attribute
// Extended attributes are not modified
//----------------------------------------
int fssSetAttr(char *fldName, int attr)
{
    int ix;
    struct sFields *fields;

    ix = findField(fldName);                // Find Field by Name
    if(ix == 0)
        return (char *) 0;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;

    // Replace Basic 3270 Attribute data
    fields[ix].attr = fssAttr(attr);

    return 0;
}

//----------------------------------------
// Replace Field Color Attribute
//
//
//----------------------------------------
int fssSetColor(char *fldName, int color)
{
    int ix;
    int attr;

    struct sFields *fields;

    ix = findField(fldName);                // Find Field by Name
    if(ix == 0)
        return (char *) 0;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;

    // Update Color Attribute Value
    attr = (fields[ix].attr & 0xFF00FF) | (color & 0xFF00);
    fields[ix].attr = attr;

    return 0;
}

//----------------------------------------
// Replace Extended Formatting Attribute
//
//
//----------------------------------------
int fssSetXH(char *fldName, int xha)
{
    int ix;
    int attr;
    struct sFields *fields;

    ix = findField(fldName);                // Find Field by Name
    if(ix == 0)
        return (char *) 0;

    // a static field?
    if(ix < 0 && isStatic)
    {
        fields = fssStaticFields;

        ix = ix * -1;
    } else
    {
        fields = fssFields;
    }

    ix--;

    // Update Extended Formatting Attribute
    attr = (fields[ix].attr & 0xFFFF) | (xha & 0xFF0000);
    fields[ix].attr = attr;

    return 0;
}

//----------------------------------------
//  Process a 3270 Input Data Stream
//
//
//----------------------------------------
static int doInput(char * buf, int len)
{
    int   l;
    int   bufpos;
    int   fldLen;
    char *p;
    char *s;

    p = buf;
    l = len;

    if(len < 3)                             // Must be at least 3 bytes long
    {
        fssAID = 0;
        fssCSR = 0;
        return -1;
    }

    fssAID = *p;                            // Save AID Value
    p++;                                    // Skip over AID
    l--;

    fssCSR = (int) address2offset( (*p << 8) + *(p+1) );  // Save Cursor Position

    p += 2;                                 // skip over Cursor Position
    l -= 2;

    while( l > 3 )                          // Min field length is 3 (0x11 + buffer position)
    {
        if( *p != 0x11 )                     // Expecting Start Field sequence
            return -2;

        p++;                                 // Skip over
        l--;

        bufpos = (int) address2offset( (*p << 8U) + *(p+1) );  // Get buffer position
        p  += 2;                             // Skip over
        l  -= 2;

        s   = p;                             // Save start of field data

        while( l && *p != 0x11 )             // Scan for end of field
        {
            p++;
            l--;
        }

        fldLen = p - s;                      // Calculate field position

        updtFld( bufpos, s, fldLen );        // Update field Contents
    }

    return 0;
}

//----------------------------------------
// Write Screen to TSO Terminal and
// get input
//
//----------------------------------------
int fssRefresh(int expires, int cls, int nowait)
{
    int   ba;
    int   ix;
    int   i;
    int   inLen;
    int   xHilight;
    int   xColor;
    int   BUFLEN;
    char *outBuf;
    char *inBuf;
    char *p;

    BUFLEN = (fssAlternateRows * fssAlternateCols * 2);       // Max buffer length

    outBuf = malloc(BUFLEN);                // alloc output buffer
    inBuf  = malloc(BUFLEN);                // alloc input buffer

    p = outBuf;                             // current position in 3270 data stream

    //*p++ = 0x27;                          // Escape
    if (cls) {
        *p++ = 0x7E;                        // Write/Erase Alternate
    } else {
        *p++ = 0xF1;                        // Write
    }

    *p++ = 0xC3;                            // WCC

    for(ix = 0; ix < fssFieldCnt; ix++)     // Loop through fields
    {
        ba   = (int)offset2address(fssFields[ix].bufaddr - 1, fssAlternateRows, fssAlternateCols);  // Back up one from field start position
        *p++ = 0x11;                         // SBA
        *p++ = (ba >> 8) & 0xFF;             // 3270 Buffer address
        *p++ = ba & 0xFF;                    // 3270 Buffer address
        *p++ = 0x1D;                         // Start Field
        *p++ = BA(fssFields[ix].attr);       // Basic Attribute

        xHilight = XH(fssFields[ix].attr);   // Get Extended Highlighting Attribute
        xColor   = XC(fssFields[ix].attr);   // Get Extended Color Attribute

        if(xHilight)                         // If any Extended Highlighting
        {
            *p++ = 0x28;                      // Set Attribute
            *p++ = 0x41;                      // Extended
            *p++ = xHilight;                  // Value
        }

        if(xColor)                           // If any Extended Color
        {
            *p++ = 0x28;                      // Set Attribute
            *p++ = 0x42;                      // Extended
            *p++ = xColor;                    // Value
        }

        i = 0;
        if(fssFields[ix].data)               // Insert field data contents
        {
            i    = strlen(fssFields[ix].data);  // length of data
            if(fssFields[ix].length < i)      // truncate if too long
                i = fssFields[ix].length;
            memcpy(p, fssFields[ix].data, i); // copy to 3270 data stream
            p += i;                           // update position in data stream
        }

        // End of field position except we are at the end
        if ((fssFields[ix].bufaddr + fssFields[ix].length) < (fssAlternateRows * fssAlternateCols)) {
            ba = (int) offset2address(fssFields[ix].bufaddr + fssFields[ix].length, fssAlternateRows, fssAlternateCols);
            *p++ = 0x11;                         // SBA
            *p++ = (ba >> 8) & 0xFF;             // 3270 buffer address
            *p++ = ba & 0xFF;
            *p++ = 0x1D;                         // start field
            *p++ = xlate3270(fssPROT);         // attrubute = protected

            if (xHilight || xColor)               // If field had Extended Attribute values
            {
                *p++ = 0x28;                      // Set Attrubite
                *p++ = 0x00;                      // Reset all
                *p++ = 0x00;                      //    to Default
            }
        }
    }

    if (!fssCSRPOS && fssCSR)
    {
        fssCSRPOS = offset2address(fssCSR, fssAlternateRows, fssAlternateCols);        // if no cursor position was specified,
    }                                       // use last known position

    if (fssCSRPOS)                          // if cursor position was specified
    {
        *p++ = 0x11;                        // SBA
        *p++ = (fssCSRPOS >> 8) & 0xFF;     // Buffer position
        *p++ = fssCSRPOS & 0xFF;
        *p++ = 0x13;                        // Insert Cursor
        fssCSRPOS = 0;
    }

    // Write Screen and Get Input
    do
    {
        tput_fullscr(outBuf, p-outBuf);      // Fullscreen TPUT
        if (nowait) {
            goto timeout;
        }

        if (expires==0)
        {
            inLen = tget_asis(inBuf, BUFLEN);    // TGET-ASIS
        }
        else
        {
            ix = expires / 50;
            if (ix<1) ix=1;

            for (i = 0; i < ix; i++){
                inLen = tget_nowait(inBuf, BUFLEN);    // TGET-NOWAIT
            //    gets(outBuf);
                if (inLen==-1) ;  // rc> 0 key was entered, rc=-1 timeout
                else break;
                Sleep(50);
            }
            if (inLen==-1) {
                fssAID = 4711;
                fssCSR = 0;
                goto timeout;
            }
        }
        if( *inBuf != 0x6E )                 // Check for reshow
            break;                            //   no - break out
    } while(1);                             // Display Screen until no reshow


    doInput(inBuf, inLen);                  // Process Input Data Stream

 timeout:
    free(outBuf);                           // Free Output Buffer
    free(inBuf);                            // Free Input Buffer
    return 0;
}

