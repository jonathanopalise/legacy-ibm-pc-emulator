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

#include"VideoDriverPalette13h.h"

CVideoDriverPalette13h::CVideoDriverPalette13h(uint8 *emulatedSourceAbsoluteAddress):CVideoDriverPalette(emulatedSourceAbsoluteAddress) {
	paletteIndex=0;
}

void CVideoDriverPalette13h::convertFromEmulatedToIntermediate() {

	static uint8 *in; //,*out,*out2;
	static uint32 index,index2;
	static uint32 *out;

	out=(uint32 *)intermediateBuffer;
	in=(emulatedSourceAbsoluteAddress);
	for (index=0; index<200; index++) {
		for (index2=0; index2<320; index2++) {
			out[0]=out[1]=palette[*in];
			out[640]=out[641]=altPalette[*in];
			out+=2;
			in++;

		}
		out+=640;
	}

}

CVideoDriverPalette13h::~CVideoDriverPalette13h() {

}

uint32 CVideoDriverPalette13h::catchPortOut8(uint16 portNumber,uint8 value) {
	switch(portNumber) {
	case 0x3c8:
		printf("Setting Palette Index to %d\n",value);
		paletteIndex=value*3;
		break;
	case 0x3c9:
		printf("Setting Palette Index %d Value %d\n",paletteIndex,value);
		switch(paletteIndex%3) {
		case 0:
			setPaletteEntry(paletteIndex/3, value*4, getPaletteEntry(paletteIndex+1), getPaletteEntry(paletteIndex+2));
			break;
		case 1:
			setPaletteEntry(paletteIndex/3, getPaletteEntry(paletteIndex-1), value*4, getPaletteEntry(paletteIndex+1));
			break;
		case 2:
			setPaletteEntry(paletteIndex/3, getPaletteEntry(paletteIndex-2), getPaletteEntry(paletteIndex-1), value*4);
			break;
		}
		
		paletteIndex++;
		if (paletteIndex>768) {
			paletteIndex=0;
		}
		break;
	}
	return(0);
}

uint8 CVideoDriverPalette13h::catchPortIn8(uint16 portNumber, uint8 *destination) {
	return(0);
}

uint8 CVideoDriverPalette13h::getWidthInCharacters() {
	return(40);
}

uint8 CVideoDriverPalette13h::getHeightInCharacters() {
	return(25);
}

void CVideoDriverPalette13h::writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage) {
}
