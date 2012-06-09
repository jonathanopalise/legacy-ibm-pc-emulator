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

#ifndef __CPU86_H
#define __CPU86_H

#include <string.h>
#include <stdio.h>

#include "legacytypes.h"


#define CPU_STATE_SIZE 100
#define MEMORY_SIZE 1048576
#define OS_BOOT_POINTER 0x7c00

// msb ----ODITSZ-A-P-C lsb

//#define FAST_STRING_INSTRUCTIONS

#define EA_REGS8 0
#define EA_REGS16 8
#define EA_REGS32 16 
#define EA_REGSMM 24
#define EA_REGSXMM 32

#define REG_REGS8 0
#define REG_REGS16 8
#define REG_REGS32 16
#define REG_REGSMM 24
#define REG_REGSXMM 32
#define REG_SEGREGS 40

#define REP_NONE 0
#define REP_REPE 1
#define REP_REPNE 2

#define MOVSB 0
#define MOVSW 1
#define CMPSB 2
#define CMPSW 3
#define STOSB 4
#define STOSW 5
#define LODSB 6
#define LODSW 7
#define SCASB 8
#define SCASW 9

#define SET_CARRY_8 carryFlag=(olddestvalue8>>7)!=(result8>>7);
#define SET_CARRY_16 carryFlag=(olddestvalue16>>15)!=(result16>>15);

#pragma pack(push,1)

class Cpu86 {
private:

	uint16 *activeSegment;

	uint8 **reglookup;
	uint8 **r8lookup;
	uint16 **r16lookup;
	uint32 **r32lookup;
	uint16 **sreglookup;
	uint8 *paritylookup;
	ptruint8 *ealookup;

	char *r8names;

	uint8 effectiveAddressIsRegister;
	uint8 *effectiveAddressMemoryOffset;
	uint8 regArrayOffset;
	uint8 effectiveAddressRegArrayOffset;
	uint8 modrmbyte;
	uint32 effectiveAddressMemoryLocation;
	uint8 *effectiveAddressPointer;
	uint16 effectiveAddressSegment;
	uint16 effectiveAddressOffset;
	uint16 effectiveAddress;
	uint8 *regPointer;
	uint16 instructionSize;
	uint8 earegmode;
	uint8 regmode;
	uint8 repmode;
	uint8 reptype;
	uint16 repindexmove;
	uint8 verticalRetrace;
	uint8 portReadsTillNextRetraceChange;
	uint8 keypressed;
	uint8 *lastPortValueWritten;

	sint16 directionModifier8;
	sint16 directionModifier16;

	/* JS */
	bool segmentDefault;

public:

	union {
		uint32 eax;
		union {
			uint16 ax;
			struct {
				uint8 al;
				uint8 ah;
			};
		};
	};
	union {
		uint32 ebx;
		union {
			uint16 bx;
			struct {
				uint8 bl;
				uint8 bh;
			};
		};
	};
	union {
		uint32 ecx;
		union {
			uint16 cx;
			struct {
				uint8 cl;
				uint8 ch;
			};
		};
	};
	union {
		uint32 edx;
		union {
			uint16 dx;
			struct {
				uint8 dl;
				uint8 dh;
			};
		};
	};
	union {
		uint32 esi;
		uint16 si;
	};
	union {
		uint32 edi;
		uint16 di;
	};
	union {
		uint32 eip;
		uint16 ip;
	};
	union {
		uint32 esp;
		uint16 sp;
	};
	union {
		uint32 ebp;
		uint16 bp;
	};
	uint16 cs;
	uint16 ds;
	uint16 es;
	uint16 fs;
	uint16 gs;
	uint16 ss;

	uint8 carryFlag; // 1
	uint8 parityFlag; // 4
	uint8 auxFlag; // 16
	uint8 zeroFlag; // 64
	uint8 signFlag; // 128
	uint8 trapFlag; // 256
	uint8 directionFlag; // 1024
	uint8 overflowFlag; // 2048

	uint8 *registers;

	uint8 interruptFlag; // 512
	
	uint8 *memory;
	uint8 running;
	/*uint8 *al,*ah,*bl,*bh,*cl,*ch,*dl,*dh;
	uint16 *ax,*bx,*cx,*dx,*si,*di,*bp,*ip,*sp,*cs,*ds,*es,*fs,*gs,*ss,*flags;
	uint32 *eax,*ebx,*ecx,*edx,*esi,*edi,*eip,*esp,*ebp;*/
	
	Cpu86();
	//void dumpvga();
	uint8 countBits8(uint8 theUint8);
	void execute();
	void hardwareInterrupt(const uint8 interruptNumber);
	void interrupt(const uint8 interruptNumber);
	void setInterruptVector(const uint8 intNumber,const uint16 segment,const uint16 offset);
	void reset();
	void decodemodrmbyte();
	void decodegroupmodrmbyte();
	void decodeleamodrmbyte();
	void printstatus();
	uint32 nativeInterrupt(uint8 intNumber);
	uint32 catchPortOut8(uint16 portNumber,uint8 value);
	uint8 catchPortIn8(uint16 portNumber, uint8 *destination);
	void catchPortOut16(uint16 portNumber,uint16 value);
	void catchPortIn16(uint16 portNumber, uint16 *destination);
	/* JS */
	void setabsolutememuint8(uint8 *dest,const uint8 value);
	void setabsolutememuint16(uint16 *dest,const uint16 value);
	void setabsolutememuint32(uint32 *dest,const uint32 value);
	/* JS */
	uint8 getabsolutememuint8(uint8 *where);
	uint16 getabsolutememuint16(uint16 *where);
	uint32 getabsolutememuint32(uint32 *where);
	void setmemuint8(const uint16 segment,const uint16 offset,const uint8 value);
	void setmemuint16(const uint16 segment,const uint16 offset,const uint16 value);
	void setmemuint32(const uint16 segment,const uint16 offset,const uint32 value);
	uint8 getmemuint8(const uint16 segment,const uint16 offset);
	uint16 getmemuint16(const uint16 segment,const uint16 offset);
	uint32 getmemuint32(const uint16 segment,const uint16 offset);
	uint8* getptrmemuint8(const uint16 segment,const uint16 offset);
	uint16* getptrmemuint16(const uint16 segment,const uint16 offset);
	uint32* getptrmemuint32(const uint16 segment,const uint16 offset);
	uint8 isRunningStringInstruction();
	~Cpu86();

	void printnest(void);
	/* JS */
	void printstack(void);
};

#pragma pack(pop)

#endif
