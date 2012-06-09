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

#include"VideoDriver.h"
#include"utilities.h"
#include"MemoryMap.h"

extern Cpu86 *theCPU;

CVideoDriver::CVideoDriver(uint8 *emulatedSourceAbsoluteAddress) {
	this->emulatedSourceAbsoluteAddress=emulatedSourceAbsoluteAddress;
}

CVideoDriver::~CVideoDriver() {
}

uint8 * CVideoDriver::getIntermediateBuffer() {
	return(intermediateBuffer);
}

void CVideoDriver::setCursorSize(const uint8 topScanLine, const uint8 bottomScanLine) {
}

void CVideoDriver::setCursorPosition(const uint8 page,const uint8 row,const uint8 column) {
}

void CVideoDriver::getCursorPosition(const uint8 page,CURSORPOSITIONANDSIZE *cursorPosition) {
}

void CVideoDriver::setActivePage(const uint8 activePage) {
	theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]=activePage;
}

void CVideoDriver::scrollActivePageUp(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll) {
}

void CVideoDriver::scrollActivePageDown(const uint8 top,const uint8 left,const uint8 bottom,const uint8 right,const uint8 blankSpaceAttribute,const uint8 linesToScroll) {
}

void CVideoDriver::readCharacterAndAttribute(const uint8 page,CHARACTERATTRIBUTE *characterAndAttribute) {
}

void CVideoDriver::writeCharacterAndAttribute(uint8 characterToWrite, uint8 characterAttributeOrColour,uint8 page,uint16 charactersToWrite) {
}

void CVideoDriver::writeCharacterAtCursor(const uint8 page,const uint8 character,const uint16 charactersToWrite) {
}

void CVideoDriver::setColourPalette(const uint8 mode,const uint8 data) {
}

void CVideoDriver::writeGraphicsPixel(const uint16 column, const uint16 row,const uint8 colour) {
}

uint8 CVideoDriver::readGraphicsPixel(const uint16 column, const uint16 row) {
	return(0);
}

void CVideoDriver::teletypeWriteCharacter(const uint8 characterToWrite, const uint8 foregroundColour) {
}

void CVideoDriver::returnCurrentVideoParameters(VIDEOPARAMS *videoParameters) {
}

void CVideoDriver::writeString(const uint16 segment, const uint16 offset, const uint16 length, const uint8 row, const uint8 column, const uint8 attribute, const uint8 writeStringMode) {
}

uint8 CVideoDriver::getActivePage() {
	return(theCPU->memory[MM_CURRENT_DISPLAY_PAGE_BYTE]);
}

uint32 CVideoDriver::catchPortOut8(uint16 portNumber,uint8 value) {
	return(0);
}

uint8 CVideoDriver::catchPortIn8(uint16 portNumber, uint8 *destination) {
	return(0);
}

void CVideoDriver::setCursorDisplay(int display) {
	displayCursor=display;
}
