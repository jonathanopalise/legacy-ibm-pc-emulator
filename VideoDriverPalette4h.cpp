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

#include "VideoDriverPalette4h.h"
#include "utilities.h"
#include "DebugOutput.h"
#include "MemoryMap.h"

extern CDebugOutput *theDebugOutput;

CVideoDriverPalette4h::CVideoDriverPalette4h(uint8 *emulatedSourceAbsoluteAddress):CVideoDriverPalette(emulatedSourceAbsoluteAddress) {

	glyphs=new uint8[2048];
	FILE *glyphFile;
	glyphFile=fopen("data/font88.raw","r");
	if (glyphFile==NULL) {
		printf("Unable to open glyph file\n");
	} else {
		printf("glyph file opened\n");
	}

	fread(glyphs,2048,1,glyphFile);
	fclose(glyphFile);
	
	setPaletteEntry(0,0,0,0);
	setPaletteEntry(1,0,255,255);
	setPaletteEntry(2,255,0,255);
	setPaletteEntry(3,255,255,255);
}

void CVideoDriverPalette4h::convertFromEmulatedToIntermediate() {
	static uint8 *in,*in2;
	static uint32 *out,*out2;
	static uint32 index,index2;
	static uint32 outval;

	in=(emulatedSourceAbsoluteAddress+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000));
	in2=(emulatedSourceAbsoluteAddress+0x2000+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000)); 

	out=(uint32 *)intermediateBuffer;
	out2=(uint32 *)intermediateBuffer+1280;

	for (index=0; index<100; index++) {
		for (index2=0; index2<80; index2++) {

			outval=palette[(*in)>>6];
			out[0]=out[1]=out[640]=out[641]=outval;
			out+=2;
			outval=palette[((*in)>>4)&3];
			out[0]=out[1]=out[640]=out[641]=outval;			
			out+=2;
			outval=palette[((*in)>>2)&3];
			out[0]=out[1]=out[640]=out[641]=outval;			
			out+=2;
			outval=palette[(*in)&3];
			out[0]=out[1]=out[640]=out[641]=outval;			
			out+=2;
			in++;

			outval=palette[(*in2)>>6];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;
			out2+=2;
			outval=palette[((*in2)>>4)&3];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;			
			out2+=2;
			outval=palette[((*in2)>>2)&3];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;			
			out2+=2;
			outval=palette[(*in2)&3];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;			
			out2+=2;
			in2++;
		}
		out+=1920;
		out2+=1920;
	}
}

void CVideoDriverPalette4h::putPixel(const uint16 x,const uint16 y, const uint8 color) {
	uint32 ofs;
	uint8 src;
	uint8 *videoMemory=emulatedSourceAbsoluteAddress;

	if ((x<320) && (y<200)) {
		ofs=((y>>1)*80)+(x>>2)+((y&1)*8192);
		src=videoMemory[ofs];
		src&=~((uint8)0xc0>>((x&3)*2));
		src|=(((color&3)<<6)>>((x&3)*2));
		videoMemory[ofs]=src;
	}
}

uint8 CVideoDriverPalette4h::getPixel(const uint16 x, const uint16 y) {
	uint32 ofs;
	uint8 src;
	uint8 *videoMemory=emulatedSourceAbsoluteAddress;
	uint8 val;

	val=0;
	if ((x<320) && (y<200)) {
		ofs=((y>>1)*80)+(x>>2)+((y&1)*8192);
		src=videoMemory[ofs];
		val=(src<<((x&3)<<1))>>6;
	}
	return(val);
	
}

void CVideoDriverPalette4h::drawLetter(const uint16 x,const uint16 y,const uint8 color, const uint8 code, int transparent) {
	uint16 curx,cury;
	uint8 *currentLetter=&glyphs[8*code];
	uint8 currentLetterBinary;
	transparent=0;

	//printf("transparent %d\n",transparent);
	for (cury=y; cury<(y+8); cury++) {
		currentLetterBinary=*currentLetter;
		for (curx=x; curx<(x+8); curx++) {
			if (currentLetterBinary&128) {
				if (!transparent) {
					putPixel(curx,cury,color&7);
				} else {
					putPixel(curx,cury,color);
				}
			} else {
				if (!transparent) {
					putPixel(curx,cury,color>>4);
				}
			}
			currentLetterBinary<<=1;         
		}
		currentLetter++;
	}
}

void CVideoDriverPalette4h::drawLetterAtTextCoords(const uint8 x, const uint8 y, const uint8 color, const uint8 code, int transparent) {
	printf("DrawLetterAtTextCoords(%d,%d,%d,%d,%d)\n",x,y,color,code,transparent);
	if ((x<40) && (y<25)) {
		drawLetter(x*8,y*8,3,code,transparent);
	}
}

void CVideoDriverPalette4h::writeGraphicsPixel(const uint16 column, const uint16 row,const uint8 colour) {
	putPixel(column,row,colour);
}

CVideoDriverPalette4h::~CVideoDriverPalette4h() {
	delete []glyphs;
}


uint8 CVideoDriverPalette4h::getWidthInCharacters() {
	return(40);
}

uint8 CVideoDriverPalette4h::getHeightInCharacters() {
	return(25);
}

void CVideoDriverPalette4h::writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage) {
}

uint32 CVideoDriverPalette4h::catchPortOut8(uint16 portNumber,uint8 value) {

	switch (portNumber) {
		case 0x3d9:
			//uint32 intensity=(((value>>4)&1) ? 255 : 127);

			setPaletteEntry(0,(value&1)*255,((value&2)>>1)*255,((value&4)>>2)*255);
			//setPaletteEntry(1,0,intensity,0);
			//setPaletteEntry(2,intensity,0,0);
			//setPaletteEntry(3,intensity,intensity,0);

			if (value&64) {
				setPaletteEntry(0,0,0,0);
				setPaletteEntry(1,0,255,0);
				setPaletteEntry(2,255,0,0);
				setPaletteEntry(3,255,64,0);
			} else {
				setPaletteEntry(0,0,0,0);
				setPaletteEntry(1,0,255,255);
				setPaletteEntry(2,255,0,255);
				setPaletteEntry(3,255,255,255);
			}
			break;
	}

	return(0);
}

void CVideoDriverPalette4h::writeCharacterAndAttribute(uint8 characterToWrite, uint8 characterAttributeOrColour,uint8 page,uint16 charactersToWrite) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint16 offset=cursorPosition.column+(cursorPosition.row*WIDTH_IN_CHARACTERS_4H);
	uint16 remaining=charactersToWrite;
	while (remaining) {
		drawLetterAtTextCoords(offset%WIDTH_IN_CHARACTERS_4H,offset/WIDTH_IN_CHARACTERS_4H,characterAttributeOrColour,characterToWrite,0);
		offset++;
		remaining--;
		if (offset==MEMORY_SCROLLUP_OFFSET_4H) {
			remaining=0;
		}
	}
}

void CVideoDriverPalette4h::writeCharacterAtCursor(const uint8 page,const uint8 character,const uint16 charactersToWrite) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint16 offset=cursorPosition.column+(cursorPosition.row*WIDTH_IN_CHARACTERS_4H);
	uint16 remaining=charactersToWrite;
	while (remaining) {
		drawLetterAtTextCoords(offset%WIDTH_IN_CHARACTERS_4H,offset/WIDTH_IN_CHARACTERS_4H,0,(uint8)character,1);
		offset++;
		remaining--;
		if (offset==MEMORY_SCROLLUP_OFFSET_4H) {
			remaining=0;
		}
	}

}

void CVideoDriverPalette4h::teletypeWriteCharacter(const uint8 characterToWrite, const uint8 foregroundColour) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);

	switch(characterToWrite) {
		case 0x8:
			if (cursorPosition.column>0) {
				setCursorPosition(getActivePage(),cursorPosition.column-1,cursorPosition.row);
			}
			break;
		case 0xa:
			setCursorPosition(getActivePage(),cursorPosition.column,cursorPosition.row+1);
			break;
		case 0xd:
			setCursorPosition(getActivePage(),0,cursorPosition.row);
			break;
		default:
			drawLetterAtTextCoords(cursorPosition.column,cursorPosition.row,1,characterToWrite,1);
			setCursorPosition(getActivePage(),cursorPosition.column+1,cursorPosition.row);
			getCursorPosition(getActivePage(),&cursorPosition);
			if (cursorPosition.column>WIDTH_IN_CHARACTERS_4H-1) {
				setCursorPosition(getActivePage(),0,cursorPosition.row+1);
			}
	}
	getCursorPosition(getActivePage(),&cursorPosition);
	if (cursorPosition.row>HEIGHT_IN_CHARACTERS_4H) {
		scrollActivePageUp(0,0,HEIGHT_IN_CHARACTERS_4H-1,WIDTH_IN_CHARACTERS_4H-1,0,1);
		setCursorPosition(getActivePage(),cursorPosition.column,cursorPosition.row-1);
	}

}

void CVideoDriverPalette4h::setCursorPosition(const uint8 page,const uint8 column,const uint8 row) {
	// low byte is column, high byte is row
	theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2)]=column;
	theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2+1)]=row;
}

void CVideoDriverPalette4h::getCursorPosition(const uint8 page,CURSORPOSITIONANDSIZE *cursorPosition) {
	cursorPosition->topScanLine=theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2];
	cursorPosition->bottomScanLine=theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2+1];

	// low byte is column, high byte is row
	cursorPosition->column=theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2)];
	cursorPosition->row=theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2+1)];
}

void CVideoDriverPalette4h::scrollActivePageUp(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll) {

	int x,y;
	int topMult=top<<3;
	int bottomMult=(bottom<<3)+7;
	int leftMult=left<<3;
	int rightMult=(right<<3)+7;
	for (y=topMult; y<=bottomMult; y++) {
		for (x=leftMult; x<=rightMult; x++) {
			putPixel(x,y,getPixel(x,y+8));
		}
	}

}
