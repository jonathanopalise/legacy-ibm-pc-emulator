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

#ifndef __VIDEO_DRIVER_PALETTE_13H
#define __VIDEO_DRIVER_PALETTE_13H

// vga/mcga 320x200x256

#include"VideoDriverPalette.h"
#include "cpu86.h"

extern Cpu86 *theCPU;

class CVideoDriverPalette13h:public CVideoDriverPalette {
private:
	//RGB8 *palette;
	uint16 paletteIndex;
public:
	CVideoDriverPalette13h(uint8 *emulatedSourceAbsoluteAddress);
	virtual uint32 catchPortOut8(uint16 portNumber,uint8 value);
	virtual uint8 catchPortIn8(uint16 portNumber, uint8 *destination);
	virtual uint8 getWidthInCharacters();
	virtual uint8 getHeightInCharacters();
	virtual void convertFromEmulatedToIntermediate();
	virtual void writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage);
	virtual ~CVideoDriverPalette13h(); // needs to clean up stuff allocated in constructor
};

#endif
