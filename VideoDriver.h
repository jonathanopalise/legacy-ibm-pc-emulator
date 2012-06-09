/*
    Legacy (trunk) - a portable emulator of the 8086-based IBM PC series of computers
    Copyright (C) 2005 Jonathan Thomas
    Contributions and fixes to CPU emulation by Jim Shaw

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef __VIDEO_DRIVER
#define __VIDEO_DRIVER

#include"legacytypes.h"
#include "cpu86.h"

struct EXPANDEDRGB8 {
	int r;
	int g;
	int b;
};

#define NATIVE_DISPLAY_WIDTH 320
#define NATIVE_DISPLAY_HEIGHT 240
#define NATIVE_DISPLAY_BPP 4

struct CHARACTERATTRIBUTE {
	uint8 character;
	uint8 attribute;
};

struct CURSORPOSITIONANDSIZE {
	uint8 column;
	uint8 row;
	uint8 topScanLine;
	uint8 bottomScanLine;
};

struct RGB8 {
	uint8 r;
	uint8 g;
	uint8 b;
};

struct VIDEOPARAMS {
	uint8 currentVideoMode;
	uint8 numberOfCharacterColumns;
	uint8 activePage;
};

class CVideoDriver {
protected:
	uint8 *nativeBuffer;
	uint8 *intermediateBuffer;
	uint8 displayCursor;

public:
	// needs to move back to protected once debugging complete
	uint8 *emulatedSourceAbsoluteAddress;

	CVideoDriver(uint8 *emulatedSourceAbsoluteAddress);
	virtual ~CVideoDriver();
	uint8 *getIntermediateBuffer();
	virtual void render()=0;

	// port in/out
	
	virtual uint32 catchPortOut8(uint16 portNumber,uint8 value);
	virtual uint8 catchPortIn8(uint16 portNumber, uint8 *destination);

	// need all bios functions and methods in here too
	// bios functions should have blank implementations in cvideodriver.cpp
	// this means that bios functionality can be added incrementally

	virtual void setCursorSize(const uint8 topScanLine, const uint8 bottomScanLine);
	virtual void setCursorPosition(const uint8 page,const uint8 row,const uint8 column);
	virtual void getCursorPosition(const uint8 page,CURSORPOSITIONANDSIZE *cursorPosition);
	virtual void setActivePage(const uint8 activePage);
	virtual void scrollActivePageUp(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll);
	virtual void scrollActivePageDown(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll);
	virtual void readCharacterAndAttribute(const uint8 page,CHARACTERATTRIBUTE *characterAndAttribute);
	virtual void writeCharacterAndAttribute(uint8 characterToWrite, uint8 characterAttributeOrColour,uint8 page,uint16 charactersToWrite);
	virtual void writeCharacterAtCursor(const uint8 page,const uint8 character,const uint16 charactersToWrite);
	virtual void setColourPalette(const uint8 mode,const uint8 data);
	virtual void writeGraphicsPixel(const uint16 column, const uint16 row,const uint8 colour);
	virtual uint8 readGraphicsPixel(const uint16 column, const uint16 row);
	virtual void teletypeWriteCharacter(const uint8 characterToWrite, const uint8 foregroundColour);
	virtual void returnCurrentVideoParameters(VIDEOPARAMS *videoParameters);
	virtual void writeString(const uint16 segment, const uint16 offset, const uint16 length, const uint8 row, const uint8 column, const uint8 attribute, const uint8 writeStringMode);
	virtual void setCursorDisplay(int display);

	// useful non-bios functions

	virtual uint8 getActivePage();
	
	// may also need port calls here

};

#endif
