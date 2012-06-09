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

#ifndef __VIDEO_DRIVER_GENERIC_3H
#define __VIDEO_DRIVER_GENERIC_3H

#include "VideoDriverPalette.h"
#include "cpu86.h"
#include "utilities.h"

// cga text 80x25

extern Cpu86 *theCPU;

#define GLYPH_HEIGHT_3H 8
#define GLYPH_WIDTH_3H 8
#define WIDTH_IN_CHARACTERS_3H 80
#define HEIGHT_IN_CHARACTERS_3H 25
#define NUM_GRADIENTS_3H 8
#define NUM_BACKGROUND_COLOURS_3H 8
#define NUM_FOREGROUND_COLOURS_3H 16
#define MEMORY_SCROLLUP_OFFSET_3H ((WIDTH_IN_CHARACTERS_3H*HEIGHT_IN_CHARACTERS_3H*2)-2)

class CVideoDriverPalette3h:public CVideoDriverPalette {
private:
	uint8 *textModeGradientLookup;
	uint8 *glyphs;
	uint8 *specialModeCRTCRegisters;
	uint8 *currentCRTCRegisters;
	uint16 *oldBuffer;
	uint8 pageSwitchingEnabled;
public:
	CVideoDriverPalette3h(uint8 *emulatedSourceAbsoluteAddress);
	void convertFromEmulatedToIntermediate();
	virtual ~CVideoDriverPalette3h();

	virtual uint8 getWidthInCharacters();
	virtual uint8 getHeightInCharacters();

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

	virtual void writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage);
	virtual void disablePageSwitching();
};

#endif
