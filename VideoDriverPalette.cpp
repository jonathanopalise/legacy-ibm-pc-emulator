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

#include "VideoDriverPalette.h"
#include "cpu86.h"
#include "utilities.h"
#include "legacytypes.h"
#include "MemoryMap.h"
#include <SDL.h>

extern SDL_Surface *screen;
extern Cpu86 *theCPU;
extern Uint8 *ubuff8;

CVideoDriverPalette::CVideoDriverPalette(uint8 *emulatedSourceAbsoluteAddress):CVideoDriver(emulatedSourceAbsoluteAddress) {
	printf("allocating intermediate display buffer of %d bytes\n",NATIVE_DISPLAY_WIDTH*NATIVE_DISPLAY_HEIGHT);
	intermediateBuffer=new uint8[1280*400*4];
	palette=new uint32[256];
	altPalette=new uint32[256];
}

void CVideoDriverPalette::convertFromIntermediateToNative() {

	uint8 *in=(uint8 *)intermediateBuffer;
	uint32 *out=(uint32 *)screen->pixels;

	if(SDL_MUSTLOCK(screen)) {
		if(SDL_LockSurface(screen) < 0)
			return;
	}

	memcpy(out,in,640*400*4);

	if(SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	SDL_Flip(screen);
}

void CVideoDriverPalette::render() {
	convertFromEmulatedToIntermediate();
	convertFromIntermediateToNative(); // system-specific
}

void CVideoDriverPalette::setActivePage(const uint8 activePage) {
	theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]=activePage;
	printf("set active page\n");
}

CVideoDriverPalette::~CVideoDriverPalette() {
	delete []altPalette;
	delete []palette;
	delete []intermediateBuffer;

}

void CVideoDriverPalette::setPaletteEntry(const uint8 entry, const uint8 r, const uint8 g, const uint8 b) {
	palette[entry]=((uint32)r<<16)+((uint32)g<<8)+(b);
	altPalette[entry]=((uint32)(r*2/3)<<16)+((uint32)(g*2/3)<<8)+(b*2/3);
}

uint8 CVideoDriverPalette::getPaletteEntry(uint8 offset) {
	return(palette[offset/3]>>((offset%3)*8));
}
