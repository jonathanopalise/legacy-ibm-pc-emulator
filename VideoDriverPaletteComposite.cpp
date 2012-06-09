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

#include "VideoDriverPaletteComposite.h"
#include "utilities.h"
#include "DebugOutput.h"
#include "MemoryMap.h"

extern CDebugOutput *theDebugOutput;

CVideoDriverPaletteComposite::CVideoDriverPaletteComposite(uint8 *emulatedSourceAbsoluteAddress):CVideoDriverPalette(emulatedSourceAbsoluteAddress) {

	int index,index2;
	uint8 *tempPal=new uint8[48];
	uint8 r,g,b;
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
	
	tempPal[0]=0;
	tempPal[1]=0;
	tempPal[2]=0;
	tempPal[3]=0;
	tempPal[4]=0x55;
	tempPal[5]=0;
	tempPal[6]=0;
	tempPal[7]=0;
	tempPal[8]=0x55;
	tempPal[9]=0;
	tempPal[10]=0xff;
	tempPal[11]=0xff;
	tempPal[12]=0xaa;
	tempPal[13]=0x55;
	tempPal[14]=0x55;
	tempPal[15]=0xaa;
	tempPal[16]=0xaa;
	tempPal[17]=0xaa;
	tempPal[18]=0xaa;
	tempPal[19]=0;
	tempPal[20]=0xaa;
	tempPal[21]=0x55;
	tempPal[22]=0x55;
	tempPal[23]=0xff;
	tempPal[24]=0xaa;
	tempPal[25]=0;
	tempPal[26]=0;
	tempPal[27]=0;
	tempPal[28]=0xaa;
	tempPal[29]=0;
	tempPal[30]=0xaa;
	tempPal[31]=0xaa;
	tempPal[32]=0x55;
	tempPal[33]=0xaa;
	tempPal[34]=0xff;
	tempPal[35]=0x55;
	tempPal[36]=0xaa;
	tempPal[37]=0x00;
	tempPal[38]=0xaa;
	tempPal[39]=0xff;
	tempPal[40]=0xaa;
	tempPal[41]=0x55;
	tempPal[42]=0xff;
	tempPal[43]=0x55;
	tempPal[44]=0x55;
	tempPal[45]=0xff;
	tempPal[46]=0xff;
	tempPal[47]=0xff;

	for (index=0; index<16; index++) {
		for (index2=0; index2<16; index2++) {
			r=tempPal[index*3]+((tempPal[index2*3]-tempPal[index*3])/2);
			g=tempPal[index*3+1]+((tempPal[index2*3+1]-tempPal[index*3+1])/2);
			b=tempPal[index*3+2]+((tempPal[index2*3+2]-tempPal[index*3+2])/2);
			setPaletteEntry(index*16+index2,r,g,b);
		}
	}

	delete []tempPal;

}

void CVideoDriverPaletteComposite::convertFromEmulatedToIntermediate() {
	static uint8 *in,*in2;
	static uint8 inval,inval2;
	static uint32 *out,*out2;
	static uint32 index,index2;
	static uint32 outval;

	in=(emulatedSourceAbsoluteAddress+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000));
	in2=(emulatedSourceAbsoluteAddress+0x2000+((theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]&3)*16000)); 

	out=(uint32 *)intermediateBuffer;
	out2=(uint32 *)intermediateBuffer+1280;

	for (index=0; index<100; index++) {
		for (index2=0; index2<80; index2++) {

			inval=(*in)>>4;
			inval2=*in&15;

			outval=palette[(inval<<4)+inval];
			out[0]=out[1]=out[640]=out[641]=outval;
			out+=2;

			outval=palette[inval+(inval2<<4)];
			out[0]=out[1]=out[640]=out[641]=outval;
			out+=2;

			outval=palette[(((*in)&15)*16)+((*in)&15)];
			out[0]=out[1]=out[640]=out[641]=outval;
			out+=2;

			outval=palette[inval2+(*(in+1)&0xf0)];
			out[0]=out[1]=out[640]=out[641]=outval;
			out+=2;

			in++;

			outval=palette[(((*in2)>>4)*16)+((*in2)>>4)];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;
			out2+=2;

			outval=palette[((*in2)>>4)+((*in2&15)*16)];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;
			out2+=2;

			outval=palette[(((*in2)&15)*16)+((*in2)&15)];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;
			out2+=2;

			outval=palette[((*in2)&15)+(((*(in2+1))>>4)*16)];
			out2[0]=out2[1]=out2[640]=out2[641]=outval;
			out2+=2;

			in2++;



		}
		out+=1920;
		out2+=1920;
	}

}

void CVideoDriverPaletteComposite::putPixel(const uint16 x,const uint16 y, const uint8 color) {
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

uint8 CVideoDriverPaletteComposite::getPixel(const uint16 x, const uint16 y) {
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

void CVideoDriverPaletteComposite::drawLetter(const uint16 x,const uint16 y,const uint8 color, const uint8 code, int transparent) {
	uint16 curx,cury;
	uint8 *currentLetter=&glyphs[8*code];
	uint8 currentLetterBinary;
	transparent=0;

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

void CVideoDriverPaletteComposite::drawLetterAtTextCoords(const uint8 x, const uint8 y, const uint8 color, const uint8 code, int transparent) {
	printf("DrawLetterAtTextCoords(%d,%d,%d,%d,%d)\n",x,y,color,code,transparent);
	if ((x<40) && (y<25)) {
		drawLetter(x*8,y*8,3,code,transparent);
	}
}

void CVideoDriverPaletteComposite::writeGraphicsPixel(const uint16 column, const uint16 row,const uint8 colour) {
	putPixel(column,row,colour);
}

CVideoDriverPaletteComposite::~CVideoDriverPaletteComposite() {
	delete []glyphs;
}


uint8 CVideoDriverPaletteComposite::getWidthInCharacters() {
	return(40);
}

uint8 CVideoDriverPaletteComposite::getHeightInCharacters() {
	return(25);
}

void CVideoDriverPaletteComposite::writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage) {
}

uint32 CVideoDriverPaletteComposite::catchPortOut8(uint16 portNumber,uint8 value) {

	return(0);
}

void CVideoDriverPaletteComposite::writeCharacterAndAttribute(uint8 characterToWrite, uint8 characterAttributeOrColour,uint8 page,uint16 charactersToWrite) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint16 offset=cursorPosition.column+(cursorPosition.row*WIDTH_IN_CHARACTERS_COMPOSITE);
	uint16 remaining=charactersToWrite;
	while (remaining) {
		drawLetterAtTextCoords(offset%WIDTH_IN_CHARACTERS_COMPOSITE,offset/WIDTH_IN_CHARACTERS_COMPOSITE,characterAttributeOrColour,characterToWrite,0);
		offset++;
		remaining--;
		if (offset==MEMORY_SCROLLUP_OFFSET_COMPOSITE) {
			remaining=0;
		}
	}
}

void CVideoDriverPaletteComposite::writeCharacterAtCursor(const uint8 page,const uint8 character,const uint16 charactersToWrite) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint16 offset=cursorPosition.column+(cursorPosition.row*WIDTH_IN_CHARACTERS_COMPOSITE);
	uint16 remaining=charactersToWrite;
	while (remaining) {
		drawLetterAtTextCoords(offset%WIDTH_IN_CHARACTERS_COMPOSITE,offset/WIDTH_IN_CHARACTERS_COMPOSITE,0,(uint8)character,1);
		offset++;
		remaining--;
		if (offset==MEMORY_SCROLLUP_OFFSET_COMPOSITE) {
			remaining=0;
		}
	}

}

void CVideoDriverPaletteComposite::teletypeWriteCharacter(const uint8 characterToWrite, const uint8 foregroundColour) {
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
			if (cursorPosition.column>WIDTH_IN_CHARACTERS_COMPOSITE-1) {
				setCursorPosition(getActivePage(),0,cursorPosition.row+1);
			}
	}
	getCursorPosition(getActivePage(),&cursorPosition);
	if (cursorPosition.row>HEIGHT_IN_CHARACTERS_COMPOSITE) {
		scrollActivePageUp(0,0,HEIGHT_IN_CHARACTERS_COMPOSITE-1,WIDTH_IN_CHARACTERS_COMPOSITE-1,0,1);
		setCursorPosition(getActivePage(),cursorPosition.column,cursorPosition.row-1);
	}

}

void CVideoDriverPaletteComposite::setCursorPosition(const uint8 page,const uint8 column,const uint8 row) {
	// low byte is column, high byte is row
	theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2)]=column;
	theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2+1)]=row;
}

void CVideoDriverPaletteComposite::getCursorPosition(const uint8 page,CURSORPOSITIONANDSIZE *cursorPosition) {
	cursorPosition->topScanLine=theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2];
	cursorPosition->bottomScanLine=theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2+1];

	// low byte is column, high byte is row
	cursorPosition->column=theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2)];
	cursorPosition->row=theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2+1)];
}

void CVideoDriverPaletteComposite::scrollActivePageUp(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll) {

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
