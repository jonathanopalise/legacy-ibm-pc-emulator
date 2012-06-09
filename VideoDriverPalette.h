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

#ifndef __VIDEO_DRIVER_PALETTE
#define __VIDEO_DRIVER_PALETTE

#include "VideoDriver.h"
#include"legacytypes.h"


class CVideoDriverPalette: public CVideoDriver {
protected:

	uint32* palette;
	uint32* altPalette;

	// put system specific stuff in here, like GAPI variables or whatever

public:
	CVideoDriverPalette(uint8 *emulatedSourceAbsoluteAddress);
	virtual void render(); // needs to call the two functions below
	virtual void convertFromEmulatedToIntermediate()=0;
	virtual void convertFromIntermediateToNative();
	virtual uint8 getWidthInCharacters()=0;
	virtual uint8 getHeightInCharacters()=0;
	virtual void writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage)=0;
	virtual void setActivePage(const uint8 activePage);
	virtual void setPaletteEntry(const uint8 entry, const uint8 r, const uint8 g, const uint8 b);
	virtual uint8 getPaletteEntry(const uint8 offset);
	virtual ~CVideoDriverPalette(); // needs to clean up stuff allocated in constructor
};

#endif
