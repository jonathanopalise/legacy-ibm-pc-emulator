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

#ifndef __DISASM_H
#define __DISASM_H

#include <string.h>
#include <stdio.h>

#include "legacytypes.h"
#include "cpu86.h"
#include "VideoDriverPalette3h.h"

#define DISASM_MODRMBYTE_STANDARD 0
#define DISASM_MODRMBYTE_GROUP 1

class CDisasm86 {
private:

	char **eatable;
	char *eaStringInput;

	char **opcodeTable;
	char *opcodeStrings;
	char **groupOpcodeTable;
	char *groupOpcodeStrings;

	char *eaStrings;
	uint8 eaSize;
	uint8 regOffset;
	Cpu86 *cpu;

	sint8 tempsint8;
	uint8 opcode;
	int explicitEaSize;


public:
	// move this back to private
	CVideoDriverPalette3h *debugScreen;


	uint8 *debugScreenBuffer;
	uint16 cs;
	uint16 ip;
	uint16 instructionSize;
	uint8 modrmbyte;
	char *eaStringOutput;
	char *opcodeOutputString;
	
	CDisasm86(Cpu86 *cpu);
	~CDisasm86();
	void paint();
	void matchCSIP();
	void advanceIP();
	void decodeInstruction();
	void render();
	void writeString(int x,int y);
	void generateHex16(uint16 val);
	void generateHex8(uint8 val);
	char *stringBuffer;
	void setAreaColour(int x1,int y1,int x2,int y2,uint8 col);

private:
	void decodemodrmbyte();
};

#endif
