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

#include "VideoDriverPalette5h.h"

CVideoDriverPalette5h::CVideoDriverPalette5h(uint8 *emulatedSourceAbsoluteAddress):CVideoDriverPalette(emulatedSourceAbsoluteAddress) {
	setPaletteEntry(0,0,0,0);
	setPaletteEntry(1,85,85,85);
	setPaletteEntry(2,170,170,170);
	setPaletteEntry(3,255,255,255);

}

void CVideoDriverPalette5h::convertFromEmulatedToIntermediate() {
	static uint8 *in,*in2,*out,*out2;
	static uint32 index,index2;

	in=(emulatedSourceAbsoluteAddress+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000));
	in2=(emulatedSourceAbsoluteAddress+0x2000+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000)); 

	out=intermediateBuffer;
	out2=intermediateBuffer+320;

	for (index=0; index<100; index++) {
		for (index2=0; index2<80; index2++) {

			*out=(*in)>>6;
			out++;
			*out=((*in)>>4)&3;
			out++;	
			*out=((*in)>>2)&3;
			out++;
			*out=(*in)&3;
			out++;
			in++;

			*out2=(*in2)>>6;
			out2++;
			*out2=((*in2)>>4)&3;
			out2++;	
			*out2=((*in2)>>2)&3;
			out2++;
			*out2=(*in2)&3;
			out2++;
			in2++;

		}
		out+=320;
		out2+=320;
	}
}


CVideoDriverPalette5h::~CVideoDriverPalette5h() {
}

uint8 CVideoDriverPalette5h::getWidthInCharacters() {
	return(80);
}

uint8 CVideoDriverPalette5h::getHeightInCharacters() {
	return(25);
}

void CVideoDriverPalette5h::writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage) {
}
