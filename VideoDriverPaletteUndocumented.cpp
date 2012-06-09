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

#include "VideoDriverPaletteUndocumented.h"
#include "stdio.h"
#include "MemoryMap.h"

CVideoDriverPaletteUndocumented::~CVideoDriverPaletteUndocumented() {
	delete []oldBuffer;
	//delete []textModeRGBValues;
	delete []glyphs;
}

CVideoDriverPaletteUndocumented::CVideoDriverPaletteUndocumented(uint8 *emulatedSourceAbsoluteAddress):CVideoDriverPalette(emulatedSourceAbsoluteAddress) {

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

	for (int index=0; index<8; index++) {
		/*setPaletteEntry(index,(index&1)*127,((index&2)>>1)*127,((index&4)>>2)*127);
		setPaletteEntry(index+8,(index&1)*255,((index&2)>>1)*255,((index&4)>>2)*255);*/
		setPaletteEntry(index,((index&4)>>2)*127,((index&2)>>1)*127,(index&1)*127);
		setPaletteEntry(index+8,((index&4)>>2)*255,((index&2)>>1)*255,(index&1)*255);

	}

	// index 8 is dark grey (technically it should be black but that would be wasting a colour)
	setPaletteEntry(8,63,63,63);

	oldBuffer=new uint16[WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED];
	memset(oldBuffer,0,WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2);
}

void CVideoDriverPaletteUndocumented::convertFromEmulatedToIntermediate() {

	static uint16 inx,iny,outy;
	//static uint8 *bufferDest;
	static uint8 *sourceCharacter,*inGlyph;
	sourceCharacter=(emulatedSourceAbsoluteAddress+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2));
	uint8 character,attribute,fgcolour,bgcolour;
	uint8 currentLetterBinary;
	int index;
	static uint16 *oldBufferPointer;
	static uint16 *sourceStart;
	static uint16 oldCursorRow;
	static uint16 oldCursorColumn;

	static uint32 *out;
	static uint32 outval;

	CURSORPOSITIONANDSIZE cursorPosition;

	sourceStart=(uint16 *)(emulatedSourceAbsoluteAddress+((getActivePage()&15)*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2));
	sourceCharacter=(uint8 *)sourceStart;
	oldBufferPointer=oldBuffer;

	for (iny=0; iny<HEIGHT_IN_CHARACTERS_UNDOCUMENTED; iny++) {
		for (inx=0; inx<WIDTH_IN_CHARACTERS_UNDOCUMENTED; inx++) {
			character=sourceCharacter[0];
			if ((*(uint16 *)sourceCharacter!=*oldBufferPointer) || ((oldCursorRow==iny) && (oldCursorColumn==inx))) {

				inGlyph=&glyphs[character*GLYPH_HEIGHT_UNDOCUMENTED];

				attribute=sourceCharacter[1];
				fgcolour=attribute&15;
				bgcolour=(attribute>>4);

				out=(uint32 *)intermediateBuffer+((NATIVE_DISPLAY_WIDTH*2*iny*4)+(inx*GLYPH_WIDTH_UNDOCUMENTED));
				for (outy=0; outy<2; outy++) {

					currentLetterBinary=*inGlyph;
					for (index=0; index<GLYPH_WIDTH_UNDOCUMENTED; index++) {
						if (currentLetterBinary&128) {
							outval=palette[fgcolour];
							out[0]=out[640]=outval;
						} else {
							outval=palette[bgcolour];
							out[0]=out[640]=outval;
						}

						currentLetterBinary<<=1;
						out+=1;
					}
					inGlyph++;
					out+=1280-GLYPH_WIDTH_UNDOCUMENTED;
				}
			}
			sourceCharacter+=2;
			oldBufferPointer++;
		}

	}

	/*getCursorPosition(getActivePage(),&cursorPosition);
	if ((displayCursor) && (cursorPosition.column<WIDTH_IN_CHARACTERS_UNDOCUMENTED) && (cursorPosition.row<HEIGHT_IN_CHARACTERS_UNDOCUMENTED)) {
		bufferDest=&intermediateBuffer[((NATIVE_DISPLAY_WIDTH*GLYPH_HEIGHT_UNDOCUMENTED*cursorPosition.row)+(cursorPosition.column*GLYPH_WIDTH_UNDOCUMENTED))*4];
		for (outy=0; outy<8; outy++) {
			for (outx=0; outx<GLYPH_WIDTH_UNDOCUMENTED; outx++) {
				if (outy>5) {
					bufferDest[0]=0xff;
					bufferDest[1]=0xff;
					bufferDest[2]=0xff;
				}
				bufferDest+=4;
				inGlyph++;
			}
			bufferDest+=(NATIVE_DISPLAY_WIDTH-GLYPH_WIDTH_UNDOCUMENTED)*4;
		}

	}*/


	memcpy(oldBuffer,sourceStart,WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2);
	oldCursorColumn=cursorPosition.column;
	oldCursorRow=cursorPosition.row;


}


void CVideoDriverPaletteUndocumented::setCursorSize(const uint8 topScanLine, const uint8 bottomScanLine) {
	uint8 *cursorSizes=&(theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2]);
	cursorSizes[0]=topScanLine&0xf;
	cursorSizes[1]=bottomScanLine&0xf;
}

void CVideoDriverPaletteUndocumented::setCursorPosition(const uint8 page,const uint8 column,const uint8 row) {
	// low byte is column, high byte is row
	theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2)]=column;
	theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2+1)]=row;
}

void CVideoDriverPaletteUndocumented::getCursorPosition(const uint8 page,CURSORPOSITIONANDSIZE *cursorPosition) {
	//uint8 *cursorLocations=&(theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8]);
	cursorPosition->topScanLine=theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2];
	cursorPosition->bottomScanLine=theCPU->memory[MM_CURRENT_CURSOR_SIZE_BYTE2+1];

	// low byte is column, high byte is row
	cursorPosition->column=theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2)];
	cursorPosition->row=theCPU->memory[MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8+(page*2+1)];
}

void CVideoDriverPaletteUndocumented::setActivePage(const uint8 activePage) {
	theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]=activePage;
}

void CVideoDriverPaletteUndocumented::scrollActivePageUp(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll) {
	uint8 *activePagePtr=&(theCPU->memory[0xB8000+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2)]);
	uint16 x,y;
	uint16 offset;
	uint8 index;
	if ( (top<HEIGHT_IN_CHARACTERS_UNDOCUMENTED) &&
		 (bottom<HEIGHT_IN_CHARACTERS_UNDOCUMENTED) &&
		 (left<WIDTH_IN_CHARACTERS_UNDOCUMENTED) &&
		 (right<WIDTH_IN_CHARACTERS_UNDOCUMENTED) &&
		 (bottom>top) &&
		 (right>left)) {
		for (index=0; index<linesToScroll; index++) {
			for (y=top; y<bottom; y++) {
				for (x=left; x<=right; x++) {
					offset=((y*WIDTH_IN_CHARACTERS_UNDOCUMENTED)+x)*2;
					activePagePtr[offset]=activePagePtr[offset+WIDTH_IN_CHARACTERS_UNDOCUMENTED*2];
					activePagePtr[offset+1]=activePagePtr[(offset+WIDTH_IN_CHARACTERS_UNDOCUMENTED*2)+1];
				}
			}
			for (x=left; x<=right; x++) {
				offset=((y*WIDTH_IN_CHARACTERS_UNDOCUMENTED)+x)*2;
				activePagePtr[offset]=0;
				activePagePtr[offset+1]=blankSpaceAttribute;
			}
		}
	}
}

void CVideoDriverPaletteUndocumented::scrollActivePageDown(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll) {
	// todo
}

void CVideoDriverPaletteUndocumented::readCharacterAndAttribute(const uint8 page,CHARACTERATTRIBUTE *characterAndAttribute) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint8 *activePagePtr=&(theCPU->memory[0xB8000+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2)]);
	uint16 offset=cursorPosition.column*cursorPosition.row*2;
	characterAndAttribute->character=activePagePtr[offset];
	characterAndAttribute->attribute=activePagePtr[offset+1];
}

void CVideoDriverPaletteUndocumented::writeCharacterAndAttribute(uint8 characterToWrite, uint8 characterAttributeOrColour,uint8 page,uint16 charactersToWrite) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint8 *activePagePtr=&(theCPU->memory[0xB8000+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2)]);
	uint16 offset=(cursorPosition.row*WIDTH_IN_CHARACTERS_UNDOCUMENTED*2)+(cursorPosition.column*2);
	uint16 remaining=charactersToWrite;
	while (remaining) {
		activePagePtr[offset]=characterToWrite;
		activePagePtr[offset+1]=characterAttributeOrColour;
		offset+=2;
		remaining--;
		if (offset==MEMORY_SCROLLUP_OFFSET_UNDOCUMENTED) {
			remaining=0;
		}
	}
}

void CVideoDriverPaletteUndocumented::writeCharacterAtCursor(const uint8 page,const uint8 character,const uint16 charactersToWrite) {
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);
	uint8 *activePagePtr=&(theCPU->memory[0xB8000+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2)]);
	uint16 offset=cursorPosition.column*cursorPosition.row*2;
	uint16 remaining=charactersToWrite;
	while (remaining) {
		activePagePtr[offset]=character;
		offset+=2;
		remaining--;
		if (offset==MEMORY_SCROLLUP_OFFSET_UNDOCUMENTED) {
			remaining=0;
		}
	}
}

void CVideoDriverPaletteUndocumented::setColourPalette(const uint8 mode,const uint8 data) {
	// no borders on emulator
}

void CVideoDriverPaletteUndocumented::writeGraphicsPixel(const uint16 column, const uint16 row,const uint8 colour) {
	// applicable to text mode? need to write asm test
}

uint8 CVideoDriverPaletteUndocumented::readGraphicsPixel(const uint16 column, const uint16 row) {
	return(0);
}

void CVideoDriverPaletteUndocumented::teletypeWriteCharacter(const uint8 characterToWrite, const uint8 foregroundColour) {
	
	CURSORPOSITIONANDSIZE cursorPosition;
	getCursorPosition(getActivePage(),&cursorPosition);

	uint8 *activePagePtr=&(theCPU->memory[0xB8000+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2)]);

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
			uint16 pageOffset=((cursorPosition.row*WIDTH_IN_CHARACTERS_UNDOCUMENTED)+cursorPosition.column)*2;
			activePagePtr[pageOffset]=characterToWrite;
			activePagePtr[pageOffset+1]=(activePagePtr[pageOffset+1]&0xf0)|(foregroundColour&0x0f);
			setCursorPosition(getActivePage(),cursorPosition.column+1,cursorPosition.row);
			getCursorPosition(getActivePage(),&cursorPosition);
			if (cursorPosition.column>79) {
				setCursorPosition(getActivePage(),0,cursorPosition.row+1);
			}
	}
	getCursorPosition(getActivePage(),&cursorPosition);
	if (cursorPosition.row>24) {
		scrollActivePageUp(0,0,HEIGHT_IN_CHARACTERS_UNDOCUMENTED-1,WIDTH_IN_CHARACTERS_UNDOCUMENTED-1,0,1);
		setCursorPosition(getActivePage(),cursorPosition.column,cursorPosition.row-1);
	}


	//printf("setting cursor position to %d %d",(pageOffset/2)%WIDTH_IN_CHARACTERS_1H,(pageOffset/2)/WIDTH_IN_CHARACTERS_1H);
}

void CVideoDriverPaletteUndocumented::returnCurrentVideoParameters(VIDEOPARAMS *videoParameters) {
	videoParameters->activePage=theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE];
	videoParameters->currentVideoMode=0x3;
	videoParameters->numberOfCharacterColumns=80;
}

void CVideoDriverPaletteUndocumented::writeString(const uint16 segment, const uint16 offset, const uint16 length,const uint8 row, const uint8 column, const uint8 attribute, const uint8 writeStringMode) {

	uint16 inOffset=offset;
	uint8 *activePagePtr=&(theCPU->memory[0xB8000+(getActivePage()*WIDTH_IN_CHARACTERS_UNDOCUMENTED*HEIGHT_IN_CHARACTERS_UNDOCUMENTED*2)]);
	if ((column<WIDTH_IN_CHARACTERS_UNDOCUMENTED) && (row<HEIGHT_IN_CHARACTERS_UNDOCUMENTED)) {
		uint16 pageOffset=column*row*2;
		uint16 remaining=length;
		int inAdd=(writeStringMode>1 ? 2: 1);
		while (remaining) {
			if (inAdd==2) {
				activePagePtr[pageOffset]=theCPU->getmemuint8(segment,inOffset);
				activePagePtr[pageOffset+1]=theCPU->getmemuint8(segment,inOffset+1);
			} else {
				activePagePtr[pageOffset]=theCPU->getmemuint8(segment,inOffset);
			}
			pageOffset+=2;
			inOffset+=inAdd;
			if (pageOffset==MEMORY_SCROLLUP_OFFSET_UNDOCUMENTED) {
				scrollActivePageUp(0,0,HEIGHT_IN_CHARACTERS_UNDOCUMENTED-1,WIDTH_IN_CHARACTERS_UNDOCUMENTED-1,0,1);
				pageOffset-=WIDTH_IN_CHARACTERS_UNDOCUMENTED*2;
			}
			remaining--;
		}
		if (writeStringMode&1) {
			//printf("setting cursor position to %d %d",(pageOffset/2)/WIDTH_IN_CHARACTERS_UNDOCUMENTED,(pageOffset/2)%WIDTH_IN_CHARACTERS_UNDOCUMENTED);
			setCursorPosition(getActivePage(),(pageOffset/2)%WIDTH_IN_CHARACTERS_UNDOCUMENTED,(pageOffset/2)/WIDTH_IN_CHARACTERS_UNDOCUMENTED);
		}
	}
}

uint8 CVideoDriverPaletteUndocumented::getWidthInCharacters() {
	return(80);
}

uint8 CVideoDriverPaletteUndocumented::getHeightInCharacters() {
	return(25);
}

void CVideoDriverPaletteUndocumented::writeCharacterAtScreenPosition(uint8 xpos,uint8 ypos,uint8 foregroundColour,uint8 backgroundColour,uint8 displayPage) {
}

