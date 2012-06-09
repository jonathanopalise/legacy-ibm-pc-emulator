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

#include "VideoDriverPalette6h.h"

CVideoDriverPalette6h::CVideoDriverPalette6h(uint8 *emulatedSourceAbsoluteAddress):CVideoDriverPalette(emulatedSourceAbsoluteAddress) {
	setPaletteEntry(0,0,0,0);
	setPaletteEntry(1,255,255,255);
}

void CVideoDriverPalette6h::convertFromEmulatedToIntermediate() {
	static uint8 *in,*in2; //,*out,*out2;
	static uint32 *out,*out2;
	static uint32 index,index2;
	static uint32 outval;

	in=(emulatedSourceAbsoluteAddress+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000));
	in2=(emulatedSourceAbsoluteAddress+0x2000+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000)); 

	out=(uint32 *)intermediateBuffer;
	out2=(uint32 *)intermediateBuffer+1280;

	for (index=0; index<100; index++) {
		for (index2=0; index2<80; index2++) {

			outval=palette[(*in)>>7];
			out[0]=out[640]=outval;
			out++;
			outval=palette[((*in)>>6)&1];
			out[0]=out[640]=outval;
			out++;
			outval=palette[((*in)>>5)&1];
			out[0]=out[640]=outval;			
			out++;
			outval=palette[((*in)>>4)&1];
			out[0]=out[640]=outval;
			out++;
			outval=palette[((*in)>>3)&1];
			out[0]=out[640]=outval;			
			out++;
			outval=palette[((*in)>>2)&1];
			out[0]=out[640]=outval;			
			out++;
			outval=palette[((*in)>>1)&1];
			out[0]=out[640]=outval;			
			out++;
			outval=palette[(*in)&1];
			out[0]=out[640]=outval;			
			out++;
			in++;

			outval=palette[(*in2)>>7];
			out2[0]=out2[640]=outval;
			out2++;
			outval=palette[((*in2)>>6)&1];
			out2[0]=out2[640]=outval;			
			out2++;
			outval=palette[((*in2)>>5)&1];
			out2[0]=out2[640]=outval;			
			out2++;
			outval=palette[((*in2)>>4)&1];
			out2[0]=out2[640]=outval;
			out2++;
			outval=palette[((*in2)>>3)&1];
			out2[0]=out2[640]=outval;			
			out2++;
			outval=palette[((*in2)>>2)&1];
			out2[0]=out2[640]=outval;			
			out2++;
			outval=palette[((*in2)>>1)&1];
			out2[0]=out2[640]=outval;			
			out2++;
			outval=palette[(*in2)&1];
			out2[0]=out2[640]=outval;			
			out2++;
			in2++;

		}
		out+=1920;
		out2+=1920;
	}
}



CVideoDriverPalette6h::~CVideoDriverPalette6h() {
}

uint8 CVideoDriverPalette6h::getWidthInCharacters() {
	return(80);
}

uint8 CVideoDriverPalette6h::getHeightInCharacters() {
	return(25);
}

void CVideoDriverPalette6h::writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage) {
}
