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


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cpu86.h"
#include "MemoryMap.h"
#include "pic.h"
#include "disasm.h"
#include "DiskImage.h"

extern CPic *thePic;
extern CDisasm86 *theDisasm;
extern CDiskImage *theFloppy;
extern int state;
extern uint8 port61h;

#define MAKE_ABSOLUTE_ADDRESS(a,b) ((a*16)+b)

#define ADVANCETIME(a) {}

/*-----------------Address Calculation Functions -------------------*/
/*-------------------------- Flags ---------------------------------*/

#define SETFLAGS_BOOLEAN_8 \
	signFlag=result8>>7;\
	zeroFlag=result8==0;\
	overflowFlag=carryFlag=0;\
	parityFlag=paritylookup[result8];

#define SETFLAGS_BOOLEAN_16 \
	signFlag=result16>>15;\
	zeroFlag=result16==0;\
	overflowFlag=carryFlag=0;\
	parityFlag=paritylookup[result16&255];

#define SETFLAGS_ADD_8 \
	signFlag=result8>>7;\
	zeroFlag=result8==0;\
	overflowFlag=!!((result8^oper8)&(result8^olddestvalue8)&0x80);\
	carryFlag=result8<olddestvalue8;\
	auxFlag=((result8>=((olddestvalue8&240)+16))&&(result8<olddestvalue8));\
	parityFlag=paritylookup[result8];

#define SETFLAGS_ADD_16 \
	signFlag=result16>>15;\
	zeroFlag=result16==0;\
	carryFlag=result16<olddestvalue16;\
	overflowFlag=!!((result16^oper16)&(result16^olddestvalue16)&0x8000);\
	auxFlag=((result16>=((olddestvalue16&240)+16))&&(result16<olddestvalue16));\
	parityFlag=paritylookup[result16&255];

#define SETFLAGS_SUB_16 \
	signFlag=result16>>15;\
	zeroFlag=result16==0;\
	carryFlag=result16>olddestvalue16;\
	overflowFlag=!!((result16^olddestvalue16)&(oper16^olddestvalue16)&0x8000);\
	auxFlag=(result16<(olddestvalue16&240))&&(result16>olddestvalue16);\
	parityFlag=paritylookup[result16&255];

#define SETFLAGS_SUB_8 \
	signFlag=result8>>7;\
	zeroFlag=result8==0;\
	carryFlag=result8>olddestvalue8;\
	overflowFlag=!!((result8^olddestvalue8)&(oper8^olddestvalue8)&0x80);\
	auxFlag=(result8<(olddestvalue8&240))&&(result8>olddestvalue8);\
	parityFlag=paritylookup[result8];

/*-------------------------- Simple Arithmetic ---------------------*/

#define SIMPLEGBEB(operator)\
	olddestvalue8=*regPointer;\
	oper8=getabsolutememuint8(effectiveAddressPointer);\
	result8=olddestvalue8 operator oper8;\
	*regPointer=result8;

#define SIMPLEEBGB(operator)\
	olddestvalue8=getabsolutememuint8(effectiveAddressPointer);\
	oper8=*regPointer;\
	result8=olddestvalue8 operator oper8;\
	setabsolutememuint8(effectiveAddressPointer, result8);

#define SIMPLEEVGV(operator)\
	if (effectiveAddressIsRegister) {\
	olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);\
	} else {\
	olddestvalue16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);\
	}\
	oper16=getabsolutememuint16((uint16 *)regPointer);\
	result16=olddestvalue16 operator oper16;\
	if (effectiveAddressIsRegister) {\
	setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);\
	} else {\
	setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);\
	}\


#define SIMPLEGVEV(operator)\
	olddestvalue16=getabsolutememuint16((uint16 *)regPointer);\
	if (effectiveAddressIsRegister) {\
	oper16=getabsolutememuint16((uint16 *)effectiveAddressPointer);\
	} else {\
	oper16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);\
	}\
	result16=olddestvalue16 operator oper16;\
	setabsolutememuint16((uint16 *)regPointer,result16);

/*-------------------------- Simple Comparison----------------------*/

#define SIMPLEGBEBNOSTORE(operator)\
	olddestvalue8=*regPointer;\
	oper8=getabsolutememuint8(effectiveAddressPointer);\
	result8=olddestvalue8 operator oper8;

#define SIMPLEEBGBNOSTORE(operator)\
	olddestvalue8=getabsolutememuint8(effectiveAddressPointer);\
	oper8=*regPointer;\
	result8=olddestvalue8 operator oper8;

#define SIMPLEEVGVNOSTORE(operator)\
	olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);\
	oper16=getabsolutememuint16((uint16 *)regPointer);\
	result16=olddestvalue16 operator oper16;

#define SIMPLEGVEVNOSTORE(operator)\
	olddestvalue16=getabsolutememuint16((uint16 *)regPointer);\
	oper16=getabsolutememuint16((uint16 *)effectiveAddressPointer);\
	result16=olddestvalue16 operator oper16;

#define SETUPBB earegmode=EA_REGS8;regmode=REG_REGS8;modrmbyte=getmemuint8(cs,ip+1);decodemodrmbyte();
#define SETUPVV earegmode=EA_REGS16;regmode=REG_REGS16;modrmbyte=getmemuint8(cs,ip+1);decodemodrmbyte();

#define SETEARESULT8\
	if (effectiveAddressIsRegister) {\
		setabsolutememuint8(effectiveAddressPointer, result8);\
	} else {\
		setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);\
	}\

#define GETEAOLDDESTVALUE8\
	if (effectiveAddressIsRegister) {\
		olddestvalue8=getabsolutememuint8(effectiveAddressPointer);\
	} else {\
		olddestvalue8=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);\
	}\

#define GETEAOLDDESTVALUE16\
	if (effectiveAddressIsRegister) {\
		olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);\
	} else {\
		olddestvalue16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);\
	}\

#define SETEARESULT16\
	if (effectiveAddressIsRegister) {\
		setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);\
	} else {\
		setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);\
	}\


#define CONDITIONALJUMPSIGNED8(condition) if ( condition ) {ip+=(sint8)(getmemuint8(cs,ip+1))+2;} else {(ip)+=2;}



uint8 Cpu86::countBits8(uint8 theUint8)
{
	uint8 bitsSet;
	uint8 index;

	bitsSet=0;
	for (index=0; index<8; index++) {
		bitsSet+=theUint8&1;
		theUint8>>=1;
	}
	return !(bitsSet&1);
}

/*-----------------Absolute Memory Write Functions ------------------*/

void Cpu86::setabsolutememuint8(uint8 *dest,const uint8 value) {
	*dest=value;
}

void Cpu86::setabsolutememuint16(uint16 *dest,const uint16 value) {
	// put memory map checks here, could implement OS level memory protection
	if (((uint32)dest&1)==1) {
		*(uint8 *)dest=(uint8)(value&255);
		*(uint8 *)((uint8 *)dest+1)=(value>>8);
	} else {
		*dest=value;
	}
}

void Cpu86::setabsolutememuint32(uint32 *dest,const uint32 value) {
	// put memory map checks here, could implement OS level memory protection
	*dest=value;
}

/*-----------------Absolute Memory Read Functions -------------------*/

uint8 Cpu86::getabsolutememuint8(uint8 *where) {
	return *where;
}

uint16 Cpu86::getabsolutememuint16(uint16 *where) {
	uint16 output;
	if (((uint32)where&1)==1) {
		output=(uint16)(*(uint8 *)where)|((uint16)(*(uint8 *)((uint8 *)where+1))<<8);
	} else {
		output=*where;
	}
	return(output);
}

uint32 Cpu86::getabsolutememuint32(uint32 *where) {
		return(*where);
}

/*-----------------Relative Memory Write Functions -------------------*/

void Cpu86::setmemuint8(const uint16 segment,const uint16 offset,const uint8 value) {
	/*uint32 offset2=((segment<<4)+offset)&0xfffff;
	if ((offset2>=0x400) && (offset2<=(0x400+0xff))) {
		printf("BIOS data area write BYTE: 40:%x (value: %x)\n",offset2-0x400,value);
	}*/

	memory[((segment<<4)+offset)&0xfffff]=value;
}

void Cpu86::setmemuint16(const uint16 segment,const uint16 offset,const uint16 value)
{
	/*setmemuint8(segment,offset,(uint8)value&255);
	setmemuint8(segment,offset+1,value>>8);*/

	memory[((segment<<4)+offset)&0xfffff]=(value&255);
	memory[((segment<<4)+(offset+1))&0xfffff]=(value>>8);

	/*uint32 offset2=((segment<<4)+offset)&0xfffff;
	if ((offset2>=0x400) && (offset2<=(0x400+0xff))) {
		printf("BIOS data area write WORD: 40:%x (value: %x)\n",offset2-0x400,value);
	}*/

}


void Cpu86::setmemuint32(const uint16 segment,const uint16 offset,const uint32 value) {
	setabsolutememuint32((uint32 *)(memory+segment*16+offset),value);
}

/*-----------------Relative Memory Read Functions -------------------*/

uint8 Cpu86::getmemuint8(const uint16 segment,const uint16 offset) {
	/*uint32 offset2=((segment<<4)+offset)&0xfffff;
	if ((offset2>=0x400) && (offset2<=(0x400+0xff))) {
		printf("BIOS data area read BYTE: 40:%x\n",offset2-0x400);
	}*/

	return(memory[((segment<<4)+offset)&0xfffff]);


}

uint16 Cpu86::getmemuint16(const uint16 segment,const uint16 offset) {
	//return(((uint16)getmemuint8(segment,offset+1)<<8)|getmemuint8(segment,offset));
	/*uint32 offset2=((segment<<4)+offset)&0xfffff;
	if ((offset2>=0x400) && (offset2<=(0x400+0xff))) {
		printf("BIOS data area read WORD: 40:%x\n",offset2-0x400);
	}*/

	return(((uint16)memory[((segment<<4)+(offset+1))&0xfffff]<<8)|memory[((segment<<4)+offset)&0xfffff]);
}


uint32 Cpu86::getmemuint32(const uint16 segment,const uint16 offset) {
	return(*(uint32 *)(memory+segment*16+offset));
}

/*-----------------Address Calculation Functions -------------------*/

uint8* Cpu86::getptrmemuint8(const uint16 segment,const uint16 offset) {
	return(&memory[segment*16+offset]);
}

uint16* Cpu86::getptrmemuint16(const uint16 segment,const uint16 offset) {
	return((uint16 *)(memory+segment*16+offset));
}

uint32* Cpu86::getptrmemuint32(const uint16 segment,const uint16 offset) {
	return((uint32 *)(memory+segment*16+offset));
}

uint8 Cpu86::isRunningStringInstruction() {
	return(this->repmode);
}

void Cpu86::setInterruptVector(const uint8 intNumber,const uint16 segment,const uint16 offset) {
	setmemuint16(0,intNumber*4,offset);
	setmemuint16(0,intNumber*4+2,segment);
}


// 80386 reference - http://webster.cs.ucr.edu/Page_TechDocs/Doc386/c17.html


void Cpu86::reset() {
	
	FILE *biosImage;
	FILE *fontImage;

	printf("opening bios image...\n");
	biosImage=fopen("data/bios.img","rb");
	fread(&memory[0xf4100-1],1,512,biosImage);
	fclose(biosImage);
	printf("bios image loaded...\n");

	printf("opening font file...\n");
	fontImage=fopen("data/font88.raw","rb");
	fread(getptrmemuint8(0xf000,0xfa6e),1,2048,fontImage);
	fclose(fontImage);
	printf("font file loaded...\n");
	
	verticalRetrace=0;
	portReadsTillNextRetraceChange=5;

	repmode=REP_NONE;

	setmemuint8(0,0x78,0x5);
	setmemuint8(0,0x79,0x37);
	setmemuint8(0,0x7a,0x2);
	setmemuint8(0,0x7b,0xc);

	// floppy drive motor status

	setmemuint8(0,0x440,0);

	// floppy drive recalibration flag

	setmemuint8(0x40,0x3e,0x80);
	
	// bios memory size
	
	setmemuint16(0x0040,0x0013,640);

	setInterruptVector(0x8,0xF000,0x4104);  // Int 8 timer interrupt
	setInterruptVector(0x10,0xF000,0x410B); // Int 10 video handler
	setInterruptVector(0x11,0xF000,0x410F); // Int 11 equipment list handler
	setInterruptVector(0x12,0xF000,0x4113); // Int 12 memory size handler
	setInterruptVector(0x14,0xF000,0x4100); // Redirect cassette calls to unsupported handler
	setInterruptVector(0x15,0xF000,0x4100); // Redirect serial calls to unsupported handler
	setInterruptVector(0x13,0xF000,0x4117); // Int 13 FDD handler
	setInterruptVector(0x19,0xF000,0x411F); // Int 19 bootstrap handler
	setInterruptVector(0x1a,0xF000,0x4123); // Int 1a getsystime handler
	setInterruptVector(0x1c,0xF000,0x4127); // Int 1c timer
	setInterruptVector(0x17,0xF000,0x4128); // Int 17 printer services
	setInterruptVector(0x9,0xF000,0x412c); // Int 9 kbd strike
	setInterruptVector(0x21,0xF000,0x4130); // Int 21 DOS Services
	setInterruptVector(0x20,0xF000,0x4134); // Int 21 Program Exit
	setInterruptVector(0x16,0xF000,0x4138); // Int 16 keyboard handler
	setInterruptVector(0x1e,0x3705,0xc2);

	setmemuint8(0x3705,0xc2,0xdf);  // step rate and head unload time
	setmemuint8(0x3705,0xc3,0x2);  // head load time and dma mode
	setmemuint8(0x3705,0xc4,0x25);  // delay for motor turn off
	setmemuint8(0x3705,0xc5,2);  // bytes per sector
	if (theFloppy!=NULL) {
		setmemuint8(0x3705,0xc6,theFloppy->sectors);    // sectors per track - sector starts at 1 therefore last sector is number of sectors
	}
	setmemuint8(0x3705,0xc7,0x1b);  // intersector gap length
	setmemuint8(0x3705,0xc8,0xff); // data length
	setmemuint8(0x3705,0xc9,0x54);  // intersector gap length during format
	setmemuint8(0x3705,0xca,0xf6); // format byte value
	setmemuint8(0x3705,0xcb,0x0);  // head settling time
	setmemuint8(0x3705,0xcc,0x0);  // delay until motor at normal speed

	printf("* reset: bytes per sector: %d\n",getmemuint8(0x3705,0xc5));
	printf("* reset: sectors per track: %d\n",getmemuint8(0x3705,0xc6));

	uint16 offset=getmemuint16(0,0x1e<<2);
	uint16 segment=getmemuint16(0,(0x1e<<2)+2);
	printf("* INT 1E vectored to %x:%x:\n",segment,offset);

	// keyboard buffer

	setmemuint16(0x40,0x1c,0x1e);
	setmemuint16(0x40,0x1a,0x1e);
	setmemuint16(0x40,0x15,0xff);
	setmemuint16(0x40,0x16,0xff);

	// game patches

	setmemuint8(0x40,0x64,0xff);

	// register settings
	
	cs=ds=es=fs=gs=ss=0;
	if (theFloppy==NULL) {
		theCPU->cs=theCPU->ds=theCPU->ss=0x151f;
		theCPU->ip=0x100;
	} else {
		ip=0x7c00;
		sp=0x03fc;
	}
	
	repmode=REP_NONE;

	overflowFlag=carryFlag=parityFlag=auxFlag=zeroFlag=signFlag=directionFlag=0;
	trapFlag=interruptFlag=1;
	directionModifier8=1;
	directionModifier16=2;

	memory[MM_CURRENT_DISPLAY_PAGE_BYTE]=0;
	memset(&memory[MM_CURRENT_CURSOR_SIZE_BYTE2],0,16);

	thePic->setLines(0x00);
	port61h=0xfe;
}

void Cpu86::hardwareInterrupt(const uint8 intNumber) {
	if (interruptFlag) {
		if (repmode) {
			if (segmentDefault) {
				ip--;
			} else {
				ip-=2;
			}
			repmode=0;
		} 
		interrupt(intNumber);
		thePic->indicateNotBusy();
	}
}

void Cpu86::interrupt(const uint8 intNumber) {
	static uint16 testuint16;

	// these lines can be used to examine the int 8 handler for games that revector it
	/*if ((intNumber==8) && (getmemuint8(2,3)!=0xf0)) {
		state=1;
	}*/

	/*if (intNumber==8) {
		state=1;
	}*/


	// push flags onto stack
	testuint16=
		0xf000|
		(uint16)carryFlag|
		((uint16)parityFlag<<2)|
		((uint16)auxFlag<<4)|
		((uint16)zeroFlag<<6)|
		((uint16)signFlag<<7)|
		((uint16)trapFlag<<8)|
		((uint16)interruptFlag<<9)|
		((uint16)directionFlag<<10)|
		((uint16)overflowFlag<<11);

	//interruptFlag=0;
	trapFlag=0;
	sp-=2;
	setmemuint16(ss,sp,testuint16);
	sp-=2;
	setmemuint16(ss,sp,cs);
	sp-=2;
	setmemuint16(ss,sp,ip);
	ip=getmemuint16(0,((uint16)intNumber*4));
	cs=getmemuint16(0,((uint16)intNumber*4)+2);

}

Cpu86::~Cpu86() {
	delete []lastPortValueWritten;
	delete []paritylookup;
	delete []memory;
	delete []reglookup;
	delete []sreglookup;
	delete []r32lookup;
	delete []r16lookup;	
	delete []r8lookup;
	delete []registers;
}

Cpu86::Cpu86() {
	registers=new uint8[CPU_STATE_SIZE];
	r8lookup=new ptruint8[8];
	r16lookup=new ptruint16[8];
	r32lookup=new ptruint32[8];
	sreglookup=new ptruint16[8];
	reglookup=new ptruint8[8*8];
	memory=new unsigned char[MEMORY_SIZE];
	paritylookup=new uint8[256];
	lastPortValueWritten=new uint8[1024];

	running=1;

	r8lookup[0]=&al;
	r8lookup[1]=&cl;
	r8lookup[2]=&dl;
	r8lookup[3]=&bl;
	r8lookup[4]=&ah;
	r8lookup[5]=&ch;
	r8lookup[6]=&dh;
	r8lookup[7]=&bh;

	reglookup[0]=&al;
	reglookup[1]=&cl;
	reglookup[2]=&dl;
	reglookup[3]=&bl;
	reglookup[4]=&ah;
	reglookup[5]=&ch;
	reglookup[6]=&dh;
	reglookup[7]=&bh;

	r16lookup[0]=&ax;
	r16lookup[1]=&cx;
	r16lookup[2]=&dx;
	r16lookup[3]=&bx;
	r16lookup[4]=&sp;
	r16lookup[5]=&bp;
	r16lookup[6]=&si;
	r16lookup[7]=&di;

	reglookup[8]=(uint8 *)&ax;
	reglookup[9]=(uint8 *)&cx;
	reglookup[10]=(uint8 *)&dx;
	reglookup[11]=(uint8 *)&bx;
	reglookup[12]=(uint8 *)&sp;
	reglookup[13]=(uint8 *)&bp;
	reglookup[14]=(uint8 *)&si;
	reglookup[15]=(uint8 *)&di;

	r32lookup[0]=&eax;
	r32lookup[1]=&ecx;
	r32lookup[2]=&edx;
	r32lookup[3]=&ebx;
	r32lookup[4]=&esp;
	r32lookup[5]=&ebp;
	r32lookup[6]=&esi;
	r32lookup[7]=&edi;

	reglookup[16]=(uint8 *)&eax;
	reglookup[17]=(uint8 *)&ecx;
	reglookup[18]=(uint8 *)&edx;
	reglookup[19]=(uint8 *)&ebx;
	reglookup[20]=(uint8 *)&esp;
	reglookup[21]=(uint8 *)&ebp;
	reglookup[22]=(uint8 *)&esi;
	reglookup[23]=(uint8 *)&edi;
	
	sreglookup[0]=&es;
	sreglookup[1]=&cs;
	sreglookup[2]=&ss;
	sreglookup[3]=&ds;
	sreglookup[4]=&fs;
	sreglookup[5]=&gs;
	sreglookup[6]=&ds; // reserved
	sreglookup[7]=&ss; // reserved

	reglookup[40]=(uint8 *)&es;
	reglookup[41]=(uint8 *)&cs;
	reglookup[42]=(uint8 *)&ss;
	reglookup[43]=(uint8 *)&ds;
	reglookup[44]=(uint8 *)&fs;
	reglookup[45]=(uint8 *)&gs;
	reglookup[46]=(uint8 *)&ds; // reserved
	reglookup[47]=(uint8 *)&ss; // reserved

	for (uint16 index=0; index<256; index++) {
		paritylookup[index]=countBits8((uint8)index);
	}
}

void Cpu86::printstatus() {
	printf("[%X %X %X %X %X %X %X %X]\n",getmemuint8(cs,ip),getmemuint8(cs,(ip)+1),getmemuint8(cs,(ip)+2),getmemuint8(cs,(ip)+3),getmemuint8(cs,(ip)+4),getmemuint8(cs,(ip)+5),getmemuint8(cs,(ip)+6),getmemuint8(cs,(ip)+7));
	printf("CS:IP=%X:%X / DS=%X ES=%X FS=%X GS=%X SS=%X\n",cs,ip,ds,es,fs,gs,ss);
	printf("AX=%X CX=%X DX=%X BX=%X SP=%X BP=%X SI=%X DI=%X\n",ax,cx,dx,bx,sp,bp,si,di); 
	printf("\n"); 
}

void Cpu86::decodemodrmbyte() {

	regPointer=reglookup[regmode+((modrmbyte>>3)&7)];
	if (!segmentDefault) {
		ADVANCETIME(2)
	}
	instructionSize=2;
	if (modrmbyte>=0xc0) {
		effectiveAddressIsRegister=true;
		effectiveAddressPointer=reglookup[earegmode+(modrmbyte&7)];
	} else {
		effectiveAddressIsRegister=false;
		effectiveAddressSegment=*activeSegment;
		switch(modrmbyte&7) {
			case 0:
				effectiveAddressOffset=bx+si;
				ADVANCETIME(7)
				break;
			case 0x1:
				effectiveAddressOffset=bx+di;
				ADVANCETIME(8)
				break;
			case 0x2:
				if (segmentDefault) {
					effectiveAddressSegment=ss;
				}
				effectiveAddressOffset=bp+si;
				ADVANCETIME(8)				
				break;
			case 0x3:
				if (segmentDefault) {
					effectiveAddressSegment=ss;
				}
				effectiveAddressOffset=bp+di;
				ADVANCETIME(11)
				break;
			case 0x4:
				effectiveAddressOffset=si;
				ADVANCETIME(5)
				break;
			case 0x5:
				effectiveAddressOffset=di;
				ADVANCETIME(5)
				break;
			case 0x6:
				if (modrmbyte>0x3f) {
					if (segmentDefault) {
						effectiveAddressSegment=ss;
					}
					effectiveAddressOffset=bp;
					ADVANCETIME(5)
				} else {
					effectiveAddressOffset=getmemuint16(cs,(ip)+2);
					instructionSize=4;
					ADVANCETIME(6)
				}
				break;
			case 0x7:
				effectiveAddressOffset=bx;
				ADVANCETIME(5)
			break;
		}
		switch(modrmbyte&0xc0) {
		case 0x80:
			effectiveAddressOffset+=getmemuint16(cs,(ip)+2);
			instructionSize=4;
			ADVANCETIME(5)
			break;
		case 0x40:
			effectiveAddressOffset+=(sint8)getmemuint8(cs,(ip)+2);
			instructionSize=3;
			ADVANCETIME(5)
			break;
		}
		effectiveAddressPointer=memory+(((effectiveAddressSegment<<4)+effectiveAddressOffset)&0xfffff);
	}
}

void Cpu86::decodegroupmodrmbyte() {

	// for group instructions, effective address lies in
	// 3 least significant bits of mod rm byte
	// and the instruction itself lies in the 3 bits
	// above this

	if (modrmbyte>=0xc0) {
		instructionSize=2;
		effectiveAddressPointer=reglookup[earegmode+(modrmbyte&7)];
		effectiveAddressIsRegister=true;
	} else {
		effectiveAddressIsRegister=false;
		//effectiveAddressOffset=0;
		effectiveAddressSegment=*activeSegment;
		instructionSize=2;
		switch(modrmbyte&7) {
			case 0:
				effectiveAddressOffset=bx+si;
				ADVANCETIME(7)
				break;
			case 0x1:
				effectiveAddressOffset=bx+di;
				ADVANCETIME(8)
				break;
			case 0x2:
				/* JS */
				/* BP uses SS as default */
				if (segmentDefault) {
					effectiveAddressSegment=ss;
				}
				effectiveAddressOffset=bp+si;
				ADVANCETIME(8)				
				break;
			case 0x3:
				/* JS */
				/* BP uses SS as default */
				if (segmentDefault) {
					effectiveAddressSegment=ss;
				}
				effectiveAddressOffset=bp+di;
				ADVANCETIME(11)
				break;
			case 0x4:
				effectiveAddressOffset=si;
				ADVANCETIME(5)
				break;
			case 0x5:
				effectiveAddressOffset=di;
				ADVANCETIME(5)
				break;
			case 0x6:
				if (modrmbyte>0x3f) {
					/* JS */
					/* BP uses SS as default */
					if (segmentDefault) {
						effectiveAddressSegment=ss;
					}
					effectiveAddressOffset=bp;
					ADVANCETIME(5)
				} else {
					effectiveAddressOffset=getmemuint16(cs,(ip)+2);
					instructionSize=4;
					ADVANCETIME(6)
				}
				break;
			case 0x7:
				effectiveAddressOffset=bx;
				ADVANCETIME(5)
			break;

		}

		switch(modrmbyte&0xc0) {
		case 0x80: // 10000000
			effectiveAddressOffset+=getmemuint16(cs,(ip)+2);
			instructionSize=4;
			ADVANCETIME(5)
			break;
		case 0x40: // 01000000
			effectiveAddressOffset+=(sint8)getmemuint8(cs,(ip)+2);
			instructionSize=3;
			ADVANCETIME(5)
			break;
		}

		effectiveAddressPointer=memory+(((effectiveAddressSegment<<4)+effectiveAddressOffset)&0xfffff);
	}
	
}

void Cpu86::decodeleamodrmbyte() {
	
	regPointer=reglookup[regmode+((modrmbyte>>3)&7)];	
	
	effectiveAddressOffset=0;
	instructionSize=2;
	switch(modrmbyte&7) {
		case 0:
			effectiveAddressOffset+=bx+si;
			break;
		case 0x1:
			effectiveAddressOffset+=bx+di;
			break;
		case 0x2:
			effectiveAddressOffset+=bp+si;
			break;
		case 0x3:
			effectiveAddressOffset+=bp+di;
			break;
		case 0x4:
			effectiveAddressOffset+=si;
			break;
		case 0x5:
			effectiveAddressOffset+=di;
			break;
		case 0x6:
			if (modrmbyte>0x3f) {
				effectiveAddressOffset+=bp;
			} else {
				effectiveAddressOffset+=getmemuint16(cs,(ip)+2);
				instructionSize=4;
			}
			break;
		case 0x7:
			effectiveAddressOffset+=bx;
		break;

	}

	switch(modrmbyte&0xc0) {
	case 0x80: // 10000000
		effectiveAddressOffset+=getmemuint16(cs,(ip)+2);
		instructionSize=4;
		break;
	case 0x40: // 01000000
		effectiveAddressOffset+=(sint8)getmemuint8(cs,(ip)+2);
		instructionSize=3;
		break;
	}

}

void Cpu86::execute() {
	static uint8 instruct;
	static uint8 testuint8,testuint81;
	static uint8 olddestvalue8;
	static uint16 index;

	static uint16 testuint16,testuint161;
	static sint16 testsint16,testsint161;
	static uint32 testuint32,testuint321;
	static sint32 testsint32,testsint321;
	static uint16 *dest16ptr,*src16ptr;
	static uint16 olddestvalue16;

	static uint8 result8;
	static uint16 result16;
	static uint32 result32;

	static uint8 oper8;
	static uint16 oper16;

	if (repmode!=REP_NONE) {
		switch(reptype) {
			case MOVSB:
				if (cx!=0) {
					cx--;
					setmemuint8(es,di,getmemuint8(*activeSegment,si));
					si+=directionModifier8;
					di+=directionModifier8;
				} else {
					repmode = REP_NONE;
					ip++;
				}
			break;
			case MOVSW:
				if (cx!=0) {
					cx--;
					setmemuint16(es,di,getmemuint16(*activeSegment,si));
					si+=directionModifier16;
					di+=directionModifier16;
				} else {
					repmode = REP_NONE;
					ip++;
				}
			break;
			case CMPSB:
				switch(repmode) {
					case REP_REPE:
						if (cx==0) {
							repmode=REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue8=getmemuint8(*activeSegment,si);
							oper8=getmemuint8(es,di);
							result8=olddestvalue8-oper8;
							SETFLAGS_SUB_8
							si+=directionModifier8;
							di+=directionModifier8;
							if (zeroFlag == 0) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
					case REP_REPNE:
						if (cx==0) {
							repmode = REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue8=getmemuint8(*activeSegment,si);
							oper8=getmemuint8(es,di);
							result8=olddestvalue8-oper8;
							SETFLAGS_SUB_8
							si+=directionModifier8;
							di+=directionModifier8;
							if (zeroFlag==1) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
					
				}
			break;
			case CMPSW:
				switch(repmode) {
					case REP_REPE:
						if (cx==0) {
							repmode=REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue16=getmemuint16(*activeSegment,si);
							oper16=getmemuint16(es,di);
							result16=olddestvalue16-oper16;
							SETFLAGS_SUB_16
							si+=directionModifier16;
							di+=directionModifier16;
							if (zeroFlag == 0) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
					case REP_REPNE:
						if (cx==0) {
							repmode=REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue16=getmemuint16(*activeSegment,si);
							oper16=getmemuint16(es,di);
							result16=olddestvalue16-oper16;
							SETFLAGS_SUB_16
							si+=directionModifier16;
							di+=directionModifier16;
							if (zeroFlag==1) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
				}
			break;
			case STOSB:
				if (cx==0) {
					repmode = REP_NONE;
					ip++;
				} else {
					cx--;
					setmemuint8(es,di,al);
					di+=directionModifier8;
				}
			break;
			case STOSW:
				if (cx==0) {
					repmode = REP_NONE;
					ip++;
				} else {
					cx--;
					setmemuint16(es,di,ax);
					di+=directionModifier16;
				}
			break;
			case SCASB:
				switch(repmode) {
					case REP_REPE:
						if (cx==0) {
							repmode = REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue8=al;
							oper8=getmemuint8(es,di);
							result8=olddestvalue8-oper8;
							SETFLAGS_SUB_8
							di+=directionModifier8;
							if (zeroFlag==1) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
					case REP_REPNE:
						if (cx==0) {
							repmode=REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue8=al;
							oper8=getmemuint8(es,di);
							result8=olddestvalue8-oper8;
							SETFLAGS_SUB_8
							di+=directionModifier8;
							if (zeroFlag==0) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
				}
			break;
			case SCASW:
				switch(repmode) { 
					case REP_REPE:
						if (cx==0) {
							repmode = REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue16=ax;
							oper16=getmemuint16(es,di);
							result16=olddestvalue16-oper16;
							SETFLAGS_SUB_16
							di+=directionModifier16;
							if (zeroFlag==1) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
					case REP_REPNE:
						if (cx==0) {
							repmode = REP_NONE;
							ip++;
						} else {
							cx--;
							olddestvalue16=ax;
							oper16=getmemuint16(es,di);
							result16=olddestvalue16-oper16;
							SETFLAGS_SUB_16
							di+=directionModifier16;
							if (zeroFlag==0) {
								repmode=REP_NONE;
								ip++;
							}
						}
					break;
				}
			break;
		}
	} else {
		instruct=getmemuint8(cs,ip);
	
		/* JS */
		/* flag if we're using the default segment */
		segmentDefault=false;
		switch (instruct) {
		case 0x26:
			activeSegment=&es;
			ip++;
			break;
		case 0x2E:
			activeSegment=&cs;
			ip++;
			break;
		case 0x36:
			activeSegment=&ss;
			ip++;
			break;
		case 0x3E:
			activeSegment=&ds;
			ip++;
			break;
		case 0x64:
			activeSegment=&fs;
			ip++;
			break;
		case 0x65:
			activeSegment=&gs;
			ip++;
			break;
		default:
			activeSegment=&ds;
			/* JS */
			/* flag if we're using the default segment */
			segmentDefault=true;
			break;
		}

		instruct=getmemuint8(cs,ip);
		switch(instruct) {

		case 0xf2:
			repmode=REP_REPNE;
			ip++;
			switch(getmemuint8(cs,ip)) {
				case 0xa4:
					reptype=MOVSB;
				break;
				case 0xa5:
					reptype=MOVSW;
				break;
				case 0xa6:
					reptype=CMPSB;
				break;
				case 0xa7:
					reptype=CMPSW;
				break;
				case 0xaa:
					reptype=STOSB;
				break;
				case 0xab:
					reptype=STOSW;
				break;
				case 0xae:
					reptype=SCASB;
				break;
				case 0xaf:
					reptype=SCASW;
				break;
			}
			break;

		case 0xf3:
			repmode=REP_REPE;
			ip++;
			switch(getmemuint8(cs,ip)) {
				case 0xa4:
					reptype=MOVSB;
				break;
				case 0xa5:
					reptype=MOVSW;
				break;
				case 0xa6:
					reptype=CMPSB;
				break;
				case 0xa7:
					reptype=CMPSW;
				break;
				case 0xaa:
					reptype=STOSB;
				break;
				case 0xab:
					reptype=STOSW;
				break;
				case 0xae:
					reptype=SCASB;
				break;
				case 0xaf:
					reptype=SCASW;
				break;
			}
			break;

		case 0: // add Eb,Gb
			ADVANCETIME(16)
			SETUPBB
			SIMPLEEBGB(+)
			SETFLAGS_ADD_8 
			ip+=instructionSize;
			break;
		case 0x1: // add Ev,Gv
			ADVANCETIME(16)
			SETUPVV
			SIMPLEEVGV(+)
			SETFLAGS_ADD_16
			ip+=instructionSize;
			break;
		case 0x2: // add Gb,Eb
			ADVANCETIME(9)
			SETUPBB
			SIMPLEGBEB(+)
			SETFLAGS_ADD_8
			ip+=instructionSize;
			break;
		case 0x3: // add Gv,Ev
			ADVANCETIME(9)
			SETUPVV
			SIMPLEGVEV(+)
			SETFLAGS_ADD_16
			ip+=instructionSize;
			break;
		case 0x4: // add al,Ib
			ADVANCETIME(4)
			olddestvalue8=al;
			oper8=getmemuint8(cs,ip+1);		
			al+=oper8;
			result8=al;
			SETFLAGS_ADD_8 
			ip+=2;
			break;
		case 0x5: // add ax,Iv
			ADVANCETIME(4)
			olddestvalue16=ax;
			oper16=getmemuint16(cs,ip+1);
			ax+=oper16;
			result16=ax;
			SETFLAGS_ADD_16
			ip+=3;
			break;
		case 0x6: // push ES
			sp-=2;
			setmemuint16(ss,sp,es);
			ip++;
			break;
		case 0x7: // pop ES
			es=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0x8: // OR Eb,Gb
			SETUPBB
			SIMPLEEBGB(|)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0x9: // OR Ev,Gv
			SETUPVV
			SIMPLEEVGV(|)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0xa: // OR Gb,Eb
			SETUPBB
			SIMPLEGBEB(|)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0xb: // OR Gv,Ev
			SETUPVV
			SIMPLEGVEV(|)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0xc: // OR AL,Ib
			al|=getmemuint8(cs,ip+1);
			result8=al;
			SETFLAGS_BOOLEAN_8
			ip+=2;
			break;
		case 0xd: // OR AX,Iv
			ax|=getmemuint16(cs,ip+1);
			result16=ax;
			SETFLAGS_BOOLEAN_16
			ip+=3;
			break;
		case 0xe: // push cs
			sp-=2;
			setmemuint16(ss,sp,cs);
			ip++;
			break;
		case 0xf:
			// pop cs (8086 only, this would be extended instructions on 286+)
			cs=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0x10: // adc Eb,Gb
			SETUPBB
			GETEAOLDDESTVALUE8
			oper8=*regPointer+carryFlag;
			result8=olddestvalue8+oper8;
			SETEARESULT8
			SETFLAGS_ADD_8 
			ip+=instructionSize;
			break;
		case 0x11: // ADC Ev,Gv
			ADVANCETIME(16);
			SETUPVV
			GETEAOLDDESTVALUE16
			oper16=(getabsolutememuint16((uint16 *)regPointer))+carryFlag;
			result16=olddestvalue16+oper16;
			SETEARESULT16
			SETFLAGS_ADD_16
			ip+=instructionSize;
			break;
		case 0x12: // ADC Gb,Eb
			ADVANCETIME(9);
			SETUPBB
			olddestvalue8=*regPointer;
			if (effectiveAddressIsRegister) {
				oper8=(getabsolutememuint8(effectiveAddressPointer))+carryFlag;
			} else {
				oper8=(getmemuint8(effectiveAddressSegment,effectiveAddressOffset))+carryFlag;
			}
			result8=olddestvalue8+oper8;
			*regPointer=result8;
			SETFLAGS_ADD_8 
			ip+=instructionSize;
			break;
		case 0x13: // ADC Gv,Ev
			ADVANCETIME(9);
			SETUPVV
			olddestvalue16=getabsolutememuint16((uint16*)regPointer);
			if (effectiveAddressIsRegister) {
				oper16=(getabsolutememuint16((uint16 *)effectiveAddressPointer))+carryFlag;
			} else {
				oper16=(getmemuint16(effectiveAddressSegment,effectiveAddressOffset))+carryFlag;
			}
			result16=olddestvalue16+oper16;
			setabsolutememuint16((uint16 *)regPointer,result16);
			SETFLAGS_ADD_16
			ip+=instructionSize;
			break;
		case 0x14: // ADC AL,Ib
			ADVANCETIME(4);
			olddestvalue8=al;
			oper8=getmemuint8(cs,ip+1)+carryFlag;
			al+=oper8;
			result8=al;
			SETFLAGS_ADD_8 
			ip+=2;
			break;
		case 0x15: // ADC AX,Iv
			ADVANCETIME(4);
			olddestvalue16=ax;
			oper16=getmemuint16(cs,ip+1)+carryFlag;
			ax+=oper16;
			result16=ax;
			SETFLAGS_ADD_16
			ip+=3;
			break;
		case 0x16: // push ss
			sp-=2;
			setmemuint16(ss,sp,ss);
			ip++;
			break;
		case 0x17://pop SS
			ss=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0x18: // SBB Eb,Gb
			SETUPBB
			GETEAOLDDESTVALUE8
			oper8=((*regPointer)+carryFlag);
			result8=olddestvalue8-oper8;
			SETEARESULT8
			SETFLAGS_SUB_8
			ip+=instructionSize;
			break;
		case 0x19: // SBB Ev,Gv
			SETUPVV
			GETEAOLDDESTVALUE16
			oper16=(getabsolutememuint16((uint16 *)regPointer)+carryFlag);
			result16=olddestvalue16-oper16;
			SETEARESULT16
			SETFLAGS_SUB_16
			ip+=instructionSize;
			break;
		case 0x1a: // SBB Gb,Eb
			SETUPBB
			olddestvalue8=*regPointer;
			if (effectiveAddressIsRegister) {
				oper8=((getabsolutememuint8(effectiveAddressPointer))+carryFlag);
			} else {
				oper8=((getmemuint8(effectiveAddressSegment,effectiveAddressOffset))+carryFlag);
			}
			result8=olddestvalue8-oper8;
			setabsolutememuint8(regPointer,result8);
			SETFLAGS_SUB_8
			ip+=instructionSize;
			break;
		case 0x1b: // SBB Gv,Ev
			SETUPVV
			olddestvalue16=getabsolutememuint16((uint16 *)regPointer);
			if (effectiveAddressIsRegister) {
				oper16=(getabsolutememuint16((uint16 *)effectiveAddressPointer)+carryFlag);
			} else {
				oper16=(getmemuint16(effectiveAddressSegment,effectiveAddressOffset)+carryFlag);
			}
			result16=olddestvalue16-oper16;
			setabsolutememuint16((uint16 *)regPointer,result16);
			SETFLAGS_SUB_16
			ip+=instructionSize;
			break;
		case 0x1c: // SBB AL,Iv
			olddestvalue8=al;
			oper8=(getmemuint8(cs,(ip)+1)+carryFlag);
			al-=oper8;
			result8=al;
			SETFLAGS_SUB_8
			ip+=2;
			break;
		case 0x1d: // SBB AX,Iv
			olddestvalue16=ax;
			oper16=(getmemuint16(cs,ip+1)+carryFlag);
			ax-=oper16;
			result16=ax;
			SETFLAGS_SUB_16
			ip+=3;
			break;
		case 0x1e: // push ds
			sp-=2;
			setmemuint16(ss,sp,ds);
			ip++;
			break;
		case 0x1f: // pop ds
			ds=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0x20: // AND Eb, Gb
			SETUPBB
			SIMPLEEBGB(&)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0x21: // AND Ev, Gv
			SETUPVV
			SIMPLEEVGV(&)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0x22: // and Gb,Eb
			SETUPBB
			SIMPLEGBEB(&)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0x23: // and Gv,Ev
			SETUPVV
			SIMPLEGVEV(&)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0x24: // and al,Ib
			result8=al&getmemuint8(cs,ip+1);
			al=result8;
			SETFLAGS_BOOLEAN_8
			ip+=2;
			break;
		case 0x25: // and ax,Iv
			result16=ax&getmemuint16(cs,ip+1);
			ax=result16;
			SETFLAGS_BOOLEAN_16
			ip+=3;
			break;
		case 0x27: // DAA
			if (((al&0xf)>9) || (auxFlag)) {
				if(al+6<al)
					carryFlag |= 1;
				al+=6;
				auxFlag=1;
			} else {
				auxFlag=0;
			}
			if ((al>0x9f) || (carryFlag)) {
				al+=0x60;
				carryFlag=1;
			} else {
				carryFlag=0;
			}
			signFlag=al>>7;
			zeroFlag=al==0;
			parityFlag=paritylookup[al&255];
			ip++;
			break;
		case 0x28: // sub Eb,Gb
			SETUPBB
			SIMPLEEBGB(-)
			SETFLAGS_SUB_8
			ip+=instructionSize;
			break;
		case 0x29: // sub Ev,Gv
			SETUPVV
			SIMPLEEVGV(-)
			SETFLAGS_SUB_16
			ip+=instructionSize;
			break;
		case 0x2a: // SUB Gb,Eb
			SETUPBB
			SIMPLEGBEB(-)
			SETFLAGS_SUB_8
			ip+=instructionSize;
			break;
		case 0x2b: // SUB Gv,Ev
			SETUPVV
			SIMPLEGVEV(-)
			SETFLAGS_SUB_16
			ip+=instructionSize;
			break;
		case 0x2c: // SUB AL,Ib
			olddestvalue8=al;
			oper8=getmemuint8(cs,ip+1);
			al-=oper8;
			result8=al;
			SETFLAGS_SUB_8
			ip+=2;
			break;
		case 0x2d: // SUB AX,Iv
			olddestvalue16=ax;
			oper16=getmemuint16(cs,ip+1);
			ax-=oper16;
			result16=ax;
			SETFLAGS_SUB_16
			ip+=3;
			break;
		case 0x2f: // DAS
			if (((al&0xf)>9) | (auxFlag)) {
				if (al-6>al)
					carryFlag |= 1;
				al-=6;
				auxFlag=1;
			} else {
				auxFlag=0;
			}
			if ((al>0x9f) | (carryFlag)) {
				al-=0x60;
				carryFlag=1;
			} else {
				carryFlag=0;
			}
			signFlag=al>>7;
			zeroFlag=al==0;
			parityFlag=paritylookup[al&255];
			ip+=1;
			break;
		case 0x30: // XOR Eb,Gb
			SETUPBB
			SIMPLEEBGB(^)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0x31: // XOR Ev,Gv
			SETUPVV
			SIMPLEEVGV(^)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0x32: // xor Gb,Eb
			SETUPBB
			SIMPLEGBEB(^)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0x33: // XOR Gv,Ev
			SETUPVV
			SIMPLEGVEV(^)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0x34: // XOR AL,Ib
			result8=al^getmemuint8(cs,ip+1);
			al=result8;
			SETFLAGS_BOOLEAN_8
			ip+=2;
			break;
		case 0x35: // XOR AX,Iv
			result16=ax^getmemuint16(cs,ip+1);
			ax=result16;
			SETFLAGS_BOOLEAN_16
			ip+=3;
			break;
		case 0x37: // AAA
			ADVANCETIME(8);
			if (((al&0xf)>9) | (auxFlag)) {
				al=(al+6)&0xf;
				ah+=1;
				auxFlag=1;
				carryFlag=1;
			} else {
				carryFlag=0;
				auxFlag=0;
			}
			al&=0xf;
			ip+=1;
			break;
		case 0x38: // CMP Eb,Gb
			SETUPBB
			SIMPLEEBGBNOSTORE(-)
			SETFLAGS_SUB_8
			ip+=instructionSize;
			break;
		case 0x39: // CMP Ev,Gv
			SETUPVV
			SIMPLEEVGVNOSTORE(-)
			SETFLAGS_SUB_16
			ip+=instructionSize;
			break;
		case 0x3a: // CMP Gb,Eb
			SETUPBB
			SIMPLEGBEBNOSTORE(-)
			SETFLAGS_SUB_8
			ip+=instructionSize;
			break;
		case 0x3b: // CMP Gv,Ev
			SETUPVV
			SIMPLEGVEVNOSTORE(-)
			SETFLAGS_SUB_16
			ip+=instructionSize;
			break;
		case 0x3c: // CMP AL,Ib	
			olddestvalue8=al;
			oper8=getmemuint8(cs,ip+1);
			result8=al-oper8;
			SETFLAGS_SUB_8
			ip+=2;
			break;
		case 0x3d: // CMP AX,Iv
			olddestvalue16=ax;
			oper16=getmemuint16(cs,ip+1);
			result16=ax-oper16;
			SETFLAGS_SUB_16
			ip+=3;
			break;
		case 0x3f: // AAS
			ADVANCETIME(8);
			if (((al&0xf)>9) | (auxFlag)) {
				al=(al-6)&0xf;
				ah-=1;
				auxFlag=1;
				carryFlag=1;
			} else {
				carryFlag=0;
				auxFlag=0;
			}
			al&=0xf;
			ip+=1;
			break;
		case 0x40: // INC AX
		case 0x41: // INC CX
		case 0x42: // INC DX
		case 0x43: // INC BX
		case 0x44: // INC SP
		case 0x45: // INC BP
		case 0x46: // INC SI
		case 0x47: // INC DI
			dest16ptr=r16lookup[getmemuint8(cs,ip)&7];
			olddestvalue16=*dest16ptr;
			oper16=1;
			result16=(*dest16ptr)+oper16;
			*dest16ptr=result16;
			signFlag=result16>>15;
			zeroFlag=result16==0;
			overflowFlag=!!((result16^oper16)&(result16^olddestvalue16)&0x8000);
			auxFlag=((result16>=((olddestvalue16&240)+16))&&(result16<olddestvalue16));
			parityFlag=paritylookup[result16&255];
			ip++;
			break;
		case 0x48: // DEC AX
		case 0x49: // DEC CX
		case 0x4a: // DEC DX
		case 0x4b: // DEC BX
		case 0x4c: // DEC SP
		case 0x4d: // DEC BP
		case 0x4e: // DEC SI
		case 0x4f: // DEC DI
			dest16ptr=r16lookup[(getmemuint8(cs,ip)-8)&7];
			olddestvalue16=*dest16ptr;
			result16=(*dest16ptr)-1;
			*dest16ptr=result16;
			signFlag=result16>>15;
			zeroFlag=result16==0;
			overflowFlag=!!((result16^olddestvalue16)&(oper16^olddestvalue16)&0x8000);
			auxFlag=(result16<(olddestvalue16&240))&&(result16>olddestvalue16);
			parityFlag=paritylookup[result16&255];
			ip++;
			break;
		case 0x50: // PUSH AX
		case 0x51: // PUSH CX
		case 0x52: // PUSH DX
		case 0x53: // PUSH BX
		case 0x54: // PUSH SP
		case 0x55: // PUSH BP
		case 0x56: // PUSH SI
		case 0x57: // PUSH DI
			// been playing around with this
			// http://pdos.csail.mit.edu/6.828/2005/readings/i386/POP.htm
			if (instruct==0x54) {
				sp-=2;
				setmemuint16(ss,sp,*(r16lookup[getmemuint8(cs,ip)&7]));
			} else {
				sp-=2;
				setmemuint16(ss,sp,*(r16lookup[getmemuint8(cs,ip)&7]));
			}
			ip++;
			break;
		case 0x58: // POP AX
		case 0x59: // POP CX
		case 0x5a: // POP DX
		case 0x5b: // POP BX
		case 0x5c: // POP SP
		case 0x5d: // POP BP
		case 0x5e: // POP SI
		case 0x5f: // POP DI
			*(r16lookup[(getmemuint8(cs,ip)-8)&7])=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0x60: // PUSHA
			testuint16=sp;
			sp-=2;
			setmemuint16(ss,sp,ax);
			sp-=2;
			setmemuint16(ss,sp,cx);
			sp-=2;
			setmemuint16(ss,sp,dx);
			sp-=2;
			setmemuint16(ss,sp,bx);
			sp-=2;
			setmemuint16(ss,sp,testuint16);
			sp-=2;
			setmemuint16(ss,sp,bp);
			sp-=2;
			setmemuint16(ss,sp,si);
			sp-=2;
			setmemuint16(ss,sp,di);
			ip++;
			break;
		case 0x61: // POPA
			di=getmemuint16(ss,sp);
			sp+=2;
			si=getmemuint16(ss,sp);
			sp+=2;
			bp=getmemuint16(ss,sp);
			sp+=2;
			//*sp=getmemuint16(*ss,*sp);
			sp+=2;
			bx=getmemuint16(ss,sp);
			sp+=2;
			dx=getmemuint16(ss,sp);
			sp+=2;
			cx=getmemuint16(ss,sp);
			sp+=2;
			ax=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0x62: // BOUND
			interrupt(6);
			break;
		case 0x63: // ARPL
			interrupt(6);
			break;
		case 0x66: // OPSIZE (80386+)
			interrupt(6);
			break;
		case 0x68: // PUSH Iv
			sp-=2;
			setmemuint16(ss,sp,getmemuint16(cs,ip+1));
			ip+=3;
			break;
		case 0x69: // IMUL Gv,Ev,Iv
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			modrmbyte=getmemuint8(cs,ip+1);
			decodemodrmbyte();
			testuint16=getmemuint16(cs,ip+instructionSize);
			result16=(*(sint16 *)effectiveAddressPointer)*((sint16)testuint16);
			setabsolutememuint16((uint16 *)regPointer,result16);
			testsint32=(*(sint16 *)effectiveAddressPointer)*((sint16)testuint16);
			if ((testsint32>32767) || (testsint32<32768)) {
				overflowFlag=carryFlag=1;
			} else {
				overflowFlag=carryFlag=0;
			}
			ip+=instructionSize+2; 
			break;
		case 0x6a: // PUSH Ib
			sp-=2;
			setmemuint16(ss,sp,getmemuint8(cs,ip+1));
			ip+=2;
			break;
		case 0x6b: // IMUL Gv,Ev,Ib
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			modrmbyte=getmemuint8(cs,ip+1);
			decodemodrmbyte();
			testuint8=getmemuint8(cs,ip+instructionSize);
			result16=(*(sint16 *)effectiveAddressPointer)*((sint8)testuint8);
			setabsolutememuint16((uint16 *)regPointer,result16);
			testsint32=(*(sint16 *)effectiveAddressPointer)*((sint8)testuint8);
			if ((testsint32>32767) || (testsint32<32768)) {
				overflowFlag=carryFlag=1;
			} else {
				overflowFlag=carryFlag=0;
			}
			ip+=instructionSize+1; 
			break;
		case 0x6c: // insb
			ip++;
			//assert(0);
			break;
		case 0x6d: // insw
			ip++;
			//assert(0);
			break;
		case 0x6e://outsb
			//assert(0);
			if (repmode==REP_NONE) {
				catchPortOut8(dx,getmemuint8(*activeSegment,si));
				si+=(directionFlag ? -1 : 1);
				ip++;
			} else {
				assert(0);
				cx--;
				if (cx==0) {
					repmode=REP_NONE;
					catchPortOut8(dx,getmemuint8(*activeSegment,si));
					si+=(directionFlag ? -1 : 1);
					ip++;
				} else {
					catchPortOut8(dx,getmemuint8(*activeSegment,si));
					si+=(directionFlag ? -1 : 1);
				}
			}
	
			break;
		case 0x6f://outsw
			//assert(0);
			if (repmode==REP_NONE) {
				catchPortOut8(dx,getmemuint8(*activeSegment,si));
				catchPortOut8(dx,getmemuint8(*activeSegment,si+1));
				si+=(directionFlag ? -2 : 2);
				ip++;
			} else {
				assert(0);
				cx--;
				if (cx==0) {
					repmode=REP_NONE;
					catchPortOut8(dx,getmemuint8(*activeSegment,si));
					catchPortOut8(dx,getmemuint8(*activeSegment,si+1));
					si+=(directionFlag ? -2 : 2);
					ip++;
				} else {
					catchPortOut8(dx,getmemuint8(*activeSegment,si));
					catchPortOut8(dx,getmemuint8(*activeSegment,si+1));
					si+=(directionFlag ? -2 : 2);
				}
			}
			break;
		case 0x70: // JO Jb
			CONDITIONALJUMPSIGNED8(overflowFlag)
			break;
		case 0x71:
			CONDITIONALJUMPSIGNED8(overflowFlag==0)
			break;
		case 0x72: // JB
			CONDITIONALJUMPSIGNED8(carryFlag)
			break;
		case 0x73: // JNB
			CONDITIONALJUMPSIGNED8(carryFlag==0)
			break;
		case 0x74: // JZ
			CONDITIONALJUMPSIGNED8(zeroFlag)
			break;
		case 0x75: // JNZ
			CONDITIONALJUMPSIGNED8(zeroFlag==0)
			break;
		case 0x76: // JBE
			CONDITIONALJUMPSIGNED8(zeroFlag || carryFlag)
			break;
		case 0x77: // JNBE
			CONDITIONALJUMPSIGNED8((zeroFlag==0) && (carryFlag==0))
			break;
		case 0x78: // JS
			CONDITIONALJUMPSIGNED8(signFlag)
			break;
		case 0x79: // JNS
			CONDITIONALJUMPSIGNED8(signFlag==0)
			break;
		case 0x7a: // JP
			CONDITIONALJUMPSIGNED8(parityFlag)
			break;
		case 0x7b: // JNP
			CONDITIONALJUMPSIGNED8(parityFlag==0)
			break;
		case 0x7c: // JL
			CONDITIONALJUMPSIGNED8(signFlag!=overflowFlag)
			break;
		case 0x7d: // JNL
			CONDITIONALJUMPSIGNED8(signFlag==overflowFlag)
			break;
		case 0x7e: // JLE
			CONDITIONALJUMPSIGNED8((signFlag!=overflowFlag) || (zeroFlag==1))
			break;
		case 0x7f: // JNLE
			CONDITIONALJUMPSIGNED8((signFlag==overflowFlag) && (zeroFlag==0))
			break;
		case 0x80:
		case 0x82: // group #1 Eb,Ib, 80 and 82 are aliases
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS8;
			regmode=REG_REGS8;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // add Eb,Ib
				GETEAOLDDESTVALUE8
				oper8=getmemuint8(cs,(ip)+instructionSize);
				result8=olddestvalue8+oper8;
				SETEARESULT8
				SETFLAGS_ADD_8
				break;
			case 0x8: // or effective address byte, immediate byte
				if (effectiveAddressIsRegister) {
					result8=getmemuint8(cs,ip+instructionSize)|getabsolutememuint8(effectiveAddressPointer);
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					result8=getmemuint8(cs,ip+instructionSize)|getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				SETFLAGS_BOOLEAN_8
				break;
			case 0x10: // adc effective address byte, immediate byte
				ADVANCETIME(17);
				GETEAOLDDESTVALUE8
				oper8=getmemuint8(cs,(ip)+instructionSize)+carryFlag;
				result8=olddestvalue8+oper8;
				SETEARESULT8
				SETFLAGS_ADD_8 
				break;
			case 0x18: // sbb effective address byte, immediate byte
				GETEAOLDDESTVALUE8
				oper8=(getmemuint8(cs,(ip)+instructionSize)+carryFlag);
				result8=olddestvalue8-oper8;
				SETEARESULT8
				SETFLAGS_SUB_8
				break;
			case 0x20: // and effective address byte, immediate byte
				if (effectiveAddressIsRegister) {
					result8=getmemuint8(cs,ip+instructionSize)&getabsolutememuint8(effectiveAddressPointer);
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					result8=getmemuint8(cs,ip+instructionSize)&getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				SETFLAGS_BOOLEAN_8
				break;
			case 0x28: // sub effective address byte, immediate byte
				GETEAOLDDESTVALUE8
				oper8=getmemuint8(cs,(ip)+instructionSize);
				result8=olddestvalue8-oper8;
				SETEARESULT8
				SETFLAGS_SUB_8
				break;
			case 0x30: // xor effective address byte, immediate byte
				if (effectiveAddressIsRegister) {
					result8=getmemuint8(cs,ip+instructionSize)^getabsolutememuint8(effectiveAddressPointer);
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					result8=getmemuint8(cs,ip+instructionSize)^getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				SETFLAGS_BOOLEAN_8
				break;
			case 0x38: // cmp effective address byte, immediate byte
				GETEAOLDDESTVALUE8
				oper8=getmemuint8(cs,(ip)+instructionSize);
				result8=olddestvalue8-oper8;
				SETFLAGS_SUB_8
				break;
			}
			ip+=instructionSize+1; // account for immediate byte
			break;
		case 0x81: // group #1 Ev,Iv
			modrmbyte=getmemuint8(cs,(ip)+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // add effective address word, immediate word
				GETEAOLDDESTVALUE16
				oper16=getmemuint16(cs,(ip)+instructionSize);
				result16=olddestvalue16+oper16;
				SETEARESULT16
				SETFLAGS_ADD_16
				break;
			case 0x8: // or effective address word, immediate word
				if (effectiveAddressIsRegister) {
					result16=getabsolutememuint16((uint16 *)effectiveAddressPointer)|getmemuint16(cs,(ip)+instructionSize);
					setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				} else {
					result16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset)|getmemuint16(cs,(ip)+instructionSize);
					setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);
				}
				SETFLAGS_BOOLEAN_16
				break;
			case 0x10: // adc effective address word, immediate word
				ADVANCETIME(17);
				GETEAOLDDESTVALUE16
				oper16=getmemuint16(cs,(ip)+instructionSize)+carryFlag;
				result16=olddestvalue16+oper16;
				SETEARESULT16
				SETFLAGS_ADD_16
				break;
			case 0x18: // sbb effective address word, immediate word
				GETEAOLDDESTVALUE16
				oper16=(getmemuint16(cs,(ip)+instructionSize)+carryFlag);
				result16=olddestvalue16-oper16;
				SETEARESULT16
				SETFLAGS_SUB_16
				break;
			case 0x20: // and effective address word, immediate word
				if (effectiveAddressIsRegister) {
					result16=getabsolutememuint16((uint16 *)effectiveAddressPointer)&getmemuint16(cs,(ip)+instructionSize);
					setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				} else {
					result16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset)&getmemuint16(cs,(ip)+instructionSize);
					setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);
				}
				SETFLAGS_BOOLEAN_16
				break;
			case 0x28: // sub effective address word, immediate word
				GETEAOLDDESTVALUE16
				oper16=getmemuint16(cs,(ip)+instructionSize);
				result16=olddestvalue16-oper16;
				SETEARESULT16
				SETFLAGS_SUB_16
				break;
			case 0x30: // xor effective address word, immediate word
				if (effectiveAddressIsRegister) {
					result16=getabsolutememuint16((uint16 *)effectiveAddressPointer)^getmemuint16(cs,(ip)+instructionSize);
					setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				} else {
					result16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset)^getmemuint16(cs,(ip)+instructionSize);
					setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);
				}
				SETFLAGS_BOOLEAN_16
				break;
			case 0x38: // cmp effective address word, immediate word
				GETEAOLDDESTVALUE16
				oper16=getmemuint16(cs,(ip)+instructionSize);
				result16=olddestvalue16-oper16;
				SETFLAGS_SUB_16
				break;
			}
			ip+=instructionSize+2;
			break;
		case 0x83: // group #1 Ev,Ib
			modrmbyte=getmemuint8(cs,(ip)+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // add effective address word, immediate byte
				GETEAOLDDESTVALUE16
				oper16=(sint8)getmemuint8(cs,(ip)+instructionSize);
				result16=olddestvalue16+oper16;
				SETEARESULT16
				SETFLAGS_ADD_16
				break;
			case 0x8: // or effective address word, immediate byte
				setabsolutememuint16((uint16 *)effectiveAddressPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer)|(sint8)getmemuint8(cs,(ip)+instructionSize));
				result16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				signFlag=result16>>15;
				zeroFlag=result16==0;
				overflowFlag=carryFlag=0;
				parityFlag=paritylookup[result16&255];
				SETFLAGS_BOOLEAN_16
				break;
			case 0x10: // adc effective address word, immediate byte
				ADVANCETIME(17);
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				oper16=(sint8)(getmemuint8(cs,(ip)+instructionSize)+carryFlag);
				result16=olddestvalue16+oper16;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				SETFLAGS_ADD_16
				break;
			case 0x18: // sbb effective address word, immediate byte
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				oper16=(sint8)(getmemuint8(cs,(ip)+instructionSize)+carryFlag);
				result16=olddestvalue16-oper16;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				SETFLAGS_SUB_16
				break;
			case 0x20: // and effective address word, immediate byte
				setabsolutememuint16((uint16 *)effectiveAddressPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer)&(sint8)getmemuint8(cs,(ip)+instructionSize));
				result16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				SETFLAGS_BOOLEAN_16
				break;
			case 0x28: // sub effective address word, immediate byte
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				oper16=(sint8)getmemuint8(cs,(ip)+instructionSize);
				result16=olddestvalue16-oper16;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				SETFLAGS_SUB_16
				break;
			case 0x30: // xor effective address word, immediate byte
				setabsolutememuint16((uint16 *)effectiveAddressPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer)^(sint8)getmemuint8(cs,(ip)+instructionSize));
				result16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				SETFLAGS_BOOLEAN_16
				break;
			case 0x38: // cmp effective address word, immediate byte
				GETEAOLDDESTVALUE16
				oper16=(sint8)getmemuint8(cs,(ip)+instructionSize);
				result16=olddestvalue16-oper16;
				SETFLAGS_SUB_16
				break;
			}
			ip+=instructionSize+1;
			break;
		case 0x84: // test Eb,Gb
			SETUPBB
			SIMPLEEBGBNOSTORE(&)
			SETFLAGS_BOOLEAN_8
			ip+=instructionSize;
			break;
		case 0x85: // test Ev,Gv
			SETUPVV
			SIMPLEEVGVNOSTORE(&)
			SETFLAGS_BOOLEAN_16
			ip+=instructionSize;
			break;
		case 0x86: // XCHG Eb,Gb
			SETUPBB
			if (effectiveAddressIsRegister) {
				testuint8=getabsolutememuint8(effectiveAddressPointer);
				setabsolutememuint8(effectiveAddressPointer, *regPointer);
			} else {
				testuint8=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				setmemuint8(effectiveAddressSegment,effectiveAddressOffset, *regPointer);
			}
			*regPointer=testuint8;
			ip+=instructionSize;
			break;
		case 0x87: // XCHG Ev,Gv
			SETUPVV
			testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
			setabsolutememuint16((uint16 *)effectiveAddressPointer,getabsolutememuint16((uint16 *)regPointer));
			setabsolutememuint16((uint16 *)regPointer,testuint16);
			ip+=instructionSize;
			break;
		case 0x88: // mov Eb,Gb
			SETUPBB
			if (effectiveAddressIsRegister) {
				setabsolutememuint8(effectiveAddressPointer, *regPointer);
			} else {
				setmemuint8(effectiveAddressSegment,effectiveAddressOffset,*regPointer);
			}
			ip+=instructionSize;
			break;
		case 0x89: // mov Ev,Gv
			SETUPVV
			if (effectiveAddressIsRegister) {
				setabsolutememuint16((uint16 *)effectiveAddressPointer,getabsolutememuint16((uint16 *)regPointer));
			} else {
				setmemuint16(effectiveAddressSegment,effectiveAddressOffset,getabsolutememuint16((uint16 *)regPointer));
			}
			ip+=instructionSize;
			break;
		case 0x8a: // mov Gb,Eb
			SETUPBB
			if (effectiveAddressIsRegister) {
				*regPointer=getabsolutememuint8(effectiveAddressPointer);
			} else {
				*regPointer=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
			}
			ip+=instructionSize;
			break;
		case 0x8b: // mov Gv,Ev
			SETUPVV
			if (effectiveAddressIsRegister) {
				setabsolutememuint16((uint16 *)regPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer));
			} else {
				setabsolutememuint16((uint16 *)regPointer,getmemuint16(effectiveAddressSegment,effectiveAddressOffset));
			}
			ip+=instructionSize;
			break;
		case 0x8c: // mov Ew,Sw
			earegmode=EA_REGS16;
			regmode=REG_SEGREGS;
			modrmbyte=getmemuint8(cs,(ip)+1);
			decodemodrmbyte();
			setabsolutememuint16((uint16 *)effectiveAddressPointer,getabsolutememuint16((uint16 *)regPointer));
			ip+=instructionSize;
			break;
		case 0x8d: // LEA Gv,M
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			modrmbyte=getmemuint8(cs,(ip)+1);
			decodeleamodrmbyte();
			setabsolutememuint16((uint16 *)regPointer,effectiveAddressOffset);
			ip+=instructionSize;
			break;
		case 0x8e: // mov Sw,Ew
			earegmode=EA_REGS16;
			regmode=REG_SEGREGS;
			modrmbyte=getmemuint8(cs,(ip)+1);
			decodemodrmbyte();
			setabsolutememuint16((uint16 *)regPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer));
			ip+=instructionSize;
			break;
		case 0x8f: // group 10 - POP Ev
			earegmode=EA_REGS16;
			regmode=REG_SEGREGS;
			modrmbyte=getmemuint8(cs,(ip)+1);
			decodegroupmodrmbyte();
			if (effectiveAddressIsRegister) {
				setabsolutememuint16((uint16 *)effectiveAddressPointer,getmemuint16(ss,sp));
			} else {
				setmemuint16(effectiveAddressSegment,effectiveAddressOffset,getmemuint16(ss,sp));
			}
			sp+=2;
			ip+=instructionSize;
			break;
		case 0x90:
			ip++;
			break;
		case 0x91: // xchg ax,[selected reg] (0x90 is NOP xchg ax,ax)
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
			src16ptr=r16lookup[getmemuint8(cs,ip)&7];
			testuint16=*src16ptr;
			*src16ptr=ax;
			ax=testuint16;
			ip++;
			break;
		case 0x98: // CBW
			// could this be done as *ax=*(sint16 *)al; ?
			//ax=(sint16)al;
			ah=(al>>7)*0xff;
			ip++;
			break;
		case 0x99: // CWD
			if ((sint16)ax<0) {
				dx=0xffff;
			} else {
				dx=0;
			}
			ip++;
			break;
		case 0x9a: // call Ap
			// cs gets pushed first
			// ip is first parameter followed by cs
			sp-=2;
			setmemuint16(ss,sp,cs);
			sp-=2;
			setmemuint16(ss,sp,ip+5);
			testuint16=cs;
			testuint161=ip;
			ip=getmemuint16(testuint16,testuint161+1);
			cs=getmemuint16(testuint16,testuint161+3);
			break;
		case 0x9b: // FWAIT
			ip++;
			//interrupt(7);
			//assert(0);
			break;
		case 0x9c: // pushf
			// msb ----ODITSZ-A-P-C lsb
			// note that 8086/8088 always sets flags 12-15 during pushf
			testuint16=0xf000|carryFlag|(parityFlag<<2)|(auxFlag<<4)|(zeroFlag<<6)|(signFlag<<7)|(trapFlag<<8)|(interruptFlag<<9)|(directionFlag<<10)|(overflowFlag<<11)|(1<<2)|(1<<13)|(1<<14)|(1<<15);
			sp-=2;
			setmemuint16(ss,sp,testuint16);
			ip++;
			break;
		case 0x9d: // popf
			testuint16=getmemuint16(ss,sp);
			sp+=2;
			carryFlag=testuint16&1;
			parityFlag=(testuint16>>2)&1;
			auxFlag=(testuint16>>4)&1;
			zeroFlag=(testuint16>>6)&1;
			signFlag=(testuint16>>7)&1;
			trapFlag=(testuint16>>8)&1;
			interruptFlag=(testuint16>>9)&1;
			directionFlag=(testuint16>>10)&1;
			overflowFlag=(testuint16>>11)&1;
			ip++;
			break;
		case 0x9e: // SAHF
			carryFlag=ah&1;
			parityFlag=(ah>>2)&1;
			auxFlag=(ah>>4)&1;
			zeroFlag=(ah>>6)&1;
			signFlag=(ah>>7)&1;
			ip++;
			break;
		case 0x9f: // lahf
			ah=carryFlag|(parityFlag<<2)|(auxFlag<<4)|(zeroFlag<<6)|(signFlag<<7)|(1<<1);
			ip++;
			break;
		case 0xa0: // mov al,Ob
			al=getmemuint8(*activeSegment,getmemuint16(cs,ip+1));
			ip+=3;
			break;
		case 0xa1: // mov ax,Ov
			ax=getmemuint16(*activeSegment,getmemuint16(cs,ip+1));
			ip+=3;
			break;
		case 0xa2: // mov Ob,al
			setmemuint8(*activeSegment,getmemuint16(cs,ip+1),al);
			ip+=3;
			break;
		case 0xa3: // mov Ov,ax
			setmemuint16(*activeSegment,getmemuint16(cs,ip+1),ax);
			ip+=3;
			break;
		case 0xa4: // (rep) movsb
			setmemuint8(es,di,getmemuint8(*activeSegment,si));
			si+=directionModifier8;
			di+=directionModifier8;
			ip++;
			break;
		case 0xa5: // rep movsw
			setmemuint16(es,di,getmemuint16(*activeSegment,si));
			si+=directionModifier16;
			di+=directionModifier16;
			ip++;
	
			break;
		case 0xa6: // cmpsb
			olddestvalue8=getmemuint8(*activeSegment,si);
			oper8=getmemuint8(es,di);
			result8=olddestvalue8-oper8;
			SETFLAGS_SUB_8
			si+=directionModifier8;
			di+=directionModifier8;
			ip++;
			break;
		case 0xa7: // cmpsw
			olddestvalue16=getmemuint16(*activeSegment,si);
			oper16=getmemuint16(es,di);
			result16=olddestvalue16-oper16;
			SETFLAGS_SUB_16
			si+=directionModifier16;
			di+=directionModifier16;
			ip++;
			break;
		case 0xa8: // TEST AL,Ib
			result8=al&getmemuint8(cs,ip+1);
			SETFLAGS_BOOLEAN_8
			ip+=2;
			break;
		case 0xa9: // TEST ax,Iv
			result16=ax&getmemuint16(cs,ip+1);
			SETFLAGS_BOOLEAN_16
			ip+=3;
			break;
		case 0xaa: // (rep) stosb
			setmemuint8(es,di,al);
			di+=directionModifier8;
			ip++;
			break;
		case 0xab: // (rep) stosw
			setmemuint16(es,di,ax);
			di+=directionModifier16;
			ip++;
			break;
		case 0xac: // lodsb
			al=getmemuint8(*activeSegment,si);
			si+=directionModifier8;
			ip++;
			break;
		case 0xad: // lodsw
			ax=getmemuint16(*activeSegment,si);
			si+=directionModifier16;
			ip++;
			break;
		case 0xae: // (rep) SCASB
			olddestvalue8=al;
			oper8=getmemuint8(es,di);
			result8=olddestvalue8-oper8;
			SETFLAGS_SUB_8
			di+=directionModifier8;
			ip++;
			break;
		case 0xaf: // (rep) scasw
			olddestvalue16=ax;
			oper16=getmemuint16(es,di);
			result16=olddestvalue16-oper16;
			SETFLAGS_SUB_16
			di+=directionModifier16;
			ip++;
			break;
		case 0xb0:
		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7: // mov reg8,imm 
			*(r8lookup[getmemuint8(cs,(ip))&7])=getmemuint8(cs,(ip)+1);
			ip+=2;
			break;
		case 0xb8:
		case 0xb9:
		case 0xba:
		case 0xbb:
		case 0xbc:
		case 0xbd:
		case 0xbe:
		case 0xbf: // mov reg16,imm
			setabsolutememuint16(r16lookup[(getmemuint8(cs,ip)-8)&7],getmemuint16(cs,ip+1));
			ip+=3;
			break;
		case 0xc0:
			modrmbyte=getmemuint8(cs,(ip)+1);
			earegmode=EA_REGS8;
			regmode=REG_REGS8;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // rol effective address byte, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				if (effectiveAddressIsRegister) {
					testuint81=getabsolutememuint8(effectiveAddressPointer);
				} else {
					testuint81=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				}
				result8=(testuint8<<testuint8)|(testuint81>>(8-testuint8));
				if (effectiveAddressIsRegister) {
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				carryFlag=result8&1;
				overflowFlag=(testuint8!=carryFlag);
				break;
			case 0x8: // ror effective address byte, immediate byte
				testuint8=getmemuint8(cs,ip+instructionSize);
				if (effectiveAddressIsRegister) {
					testuint81=getabsolutememuint8(effectiveAddressPointer);
				} else {
					testuint81=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				}
				result8=(testuint81>>testuint8)|(testuint81<<(8-testuint8));
				if (effectiveAddressIsRegister) {
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				carryFlag=testuint81&1;
				overflowFlag=(result8>>7)^((result8>>6)&1);
				break;
			case 0x10: // rcl effective address byte, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				if (effectiveAddressIsRegister) {
					testuint16=getabsolutememuint8(effectiveAddressPointer);
				} else {
					testuint16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				testuint16|=(uint16)carryFlag<<8;
				result16=(testuint16<<testuint8)|(testuint16>>(9-testuint8));
				result8=(uint8)result16;
				carryFlag=(result16>>8)&1;
				if (effectiveAddressIsRegister) {
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				if (testuint8==1) {
					overflowFlag=((result8>>7)!=carryFlag);
				}
				break;
			case 0x18: // rcr effective address byte, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				if (effectiveAddressIsRegister) {
					result8=getabsolutememuint8(effectiveAddressPointer);
				} else {
					result8=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				}
				while (testuint8) {
					testuint81=carryFlag;
					carryFlag=result8&1;
					result8>>=1;
					result8|=testuint81<<7;
					testuint8--;
				}
				if (testuint8==1) {
					if ((result8>>7)==((result8>>6)&1)) {
						overflowFlag=0;
					} else {
						overflowFlag=1;
					}
				}
				if (effectiveAddressIsRegister) {
					setabsolutememuint8(effectiveAddressPointer,result8);
				} else {
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				break;
			case 0x20:
			case 0x30: // shl effective address byte, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				olddestvalue8=getabsolutememuint8(effectiveAddressPointer);
				carryFlag=(olddestvalue8>>(8-testuint8))&1; // carryFlag is last number shifted out
				result8=olddestvalue8<<testuint8;
				setabsolutememuint8(effectiveAddressPointer, result8);
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				ip+=instructionSize+1;
				break;
			case 0x28: // shr effective address byte, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				GETEAOLDDESTVALUE8
				carryFlag=(olddestvalue8>>(testuint8-1))&1;
				result8=olddestvalue8>>testuint8;
				SETEARESULT8
				overflowFlag=result8>>7;
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				break;
			case 0x38: // sar effective address byte, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				GETEAOLDDESTVALUE8
				carryFlag=(olddestvalue8>>(testuint8-1))&1;
				result8=(sint8)olddestvalue8>>testuint8;
				SETEARESULT8
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				break;
			}
			ip+=instructionSize+1;
			break;
		case 0xc1:
			modrmbyte=getmemuint8(cs,(ip)+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // rol effective address word, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				result16=(testuint16<<testuint8)|(testuint16>>(16-testuint8));
				carryFlag=result16&1;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				overflowFlag=(testuint8!=carryFlag);
				break;
			case 0x8: // ror effective address word, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				result16=(testuint16>>testuint8)|(testuint16<<(16-testuint8));
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				carryFlag=(uint8)(testuint16&1);
				overflowFlag=(result16>>15)^((result16>>14)&1);
				break;
			case 0x10: // rcl effective address word, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				if (effectiveAddressIsRegister) {
					testuint32=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				} else {
					testuint32=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				testuint32|=(uint32)carryFlag<<16;
				result32=(testuint32<<testuint8)|(testuint32>>(17-testuint8));
				result16=(uint16)result32;
				carryFlag=(result32>>16)&1;
				if (effectiveAddressIsRegister) {
					setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				} else {
					setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);
				}
				if (testuint8==1) {
					overflowFlag=(testuint8!=carryFlag);
				}
				break;
			case 0x18: // rcr effective address word, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				if (effectiveAddressIsRegister) {
					testuint32=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				} else {
					testuint32=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				testuint32|=(uint32)carryFlag<<16;
				result32=(testuint32>>testuint8)|(testuint32<<(17-testuint8));
				result16=(uint16)result32;
				carryFlag=(result32>>16)&1;
				if (effectiveAddressIsRegister) {
					setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				} else {
					setmemuint16(effectiveAddressSegment,effectiveAddressOffset,result16);
				}
				if (testuint8==1) {
					if ((result8>>7)==((result8>>6)&1)) {
						overflowFlag=0;
					} else {
						overflowFlag=1;
					}
				}
				break;
			case 0x20:
			case 0x30: // shl effective address word, immediate byte
				testuint8=getmemuint8(cs,ip+instructionSize);
				GETEAOLDDESTVALUE16
				carryFlag=(olddestvalue16>>(16-testuint8))&1; // carryFlag is last number shifted out
				result16=olddestvalue16<<testuint8;
				SETEARESULT16
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				break;
			case 0x28: // shr effective address word, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				GETEAOLDDESTVALUE16
				carryFlag=(olddestvalue16>>(testuint8-1))&1;
				result16=olddestvalue16>>testuint8;
				SETEARESULT16
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				break;
			case 0x38: // sar effective address word, immediate byte
				testuint8=getmemuint8(cs,(ip)+instructionSize);
				GETEAOLDDESTVALUE16
				carryFlag=(olddestvalue16>>(testuint8-1))&1;
				result16=(sint16)olddestvalue16>>testuint8;
				SETEARESULT16
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				overflowFlag=0;
				break;
			}
			ip+=instructionSize+1;
			break;
		case 0xc2: // RET near lw
			testuint16=getmemuint16(cs,ip+1);
			ip=getmemuint16(ss,sp);
			sp+=2+testuint16;
			break;
		case 0xc3: // RET near
			ip=getmemuint16(ss,sp);
			sp+=2;
			break;
		case 0xc4: // les
			SETUPVV
			setabsolutememuint16((uint16 *)regPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer));
			es=getabsolutememuint16((uint16 *)(effectiveAddressPointer+2));
			ip+=instructionSize;
			break;
		case 0xc5: // lds
			SETUPVV
			setabsolutememuint16((uint16 *)regPointer,getabsolutememuint16((uint16 *)effectiveAddressPointer));
			ds=getabsolutememuint16((uint16 *)(effectiveAddressPointer+2));
			ip+=instructionSize;
			break;
		case 0xc6: // group 12 Eb,Ib
			modrmbyte=getmemuint8(cs,(ip)+1);
			if ((modrmbyte&0x38)==0) {
				earegmode=EA_REGS8;
				regmode=REG_REGS8;
				decodegroupmodrmbyte();
				if (effectiveAddressIsRegister) {
					setabsolutememuint8(effectiveAddressPointer, getmemuint8(cs,(ip)+instructionSize));
				} else {
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,getmemuint8(cs,(ip)+instructionSize));
				}
				ip+=instructionSize+1;
			} else {
				ip+=instructionSize+1;
				interrupt(6);
			}
			break;
		case 0xc7: // group 12 Ew,Iw
			modrmbyte=getmemuint8(cs,(ip)+1);
			if ((modrmbyte&0x38)==0) {
				earegmode=EA_REGS16;
				regmode=REG_REGS16;
				decodegroupmodrmbyte();
				if (effectiveAddressIsRegister) {
					setabsolutememuint16((uint16 *)effectiveAddressPointer,getmemuint16(cs,(ip)+instructionSize)); 
				} else {
					setmemuint16(effectiveAddressSegment,effectiveAddressOffset,getmemuint16(cs,(ip)+instructionSize)); 
				}
				ip+=instructionSize+2;
			} else {
				ip+=instructionSize+2;
				interrupt(6);
			}
			break;
		case 0xc8: // enter
			//assert(0);
			testuint16=getmemuint16(cs,(ip)+1); //stack allocation
			testuint8=getmemuint8(cs,(ip)+3)&31; //nesting level
			//assert(testuint8==0);//unsure about the following loop
			sp -= 2;
			setmemuint16(ss,sp,bp);
			testuint161=sp; 
			for (index=1;index<testuint8;index++) {
				bp -= 2;
				sp -= 2;
				setmemuint16(ss,sp,getmemuint16(ss,bp));
			}
			bp = testuint161;
			sp = bp - testuint16;
			ip += 4;
			break;
		case 0xc9: // leave
			sp=bp;
			bp=getmemuint16(ss,sp);
			sp+=2;
			ip++;
			break;
		case 0xca: // ret far imm16
			testuint16=getmemuint16(cs,ip+1);
			ip=getmemuint16(ss,sp);
			sp+=2;
			cs=getmemuint16(ss,sp);
			sp+=2;
			sp+=testuint16;
			break;
		case 0xcb: // ret far
			ip=getmemuint16(ss,sp);
			sp+=2;
			cs=getmemuint16(ss,sp);
			sp+=2;
			break;
		case 0xcc: // int 3
			ip++;
			// int3 disabled, rasterscan uses it but doesnt set the vector
			// so it crashes
			//interrupt(3);
			break;
		case 0xcd: // int immediate
			testuint8=getmemuint8(cs,ip+1);
			ip+=2;
			interrupt(testuint8);
			break;
		case 0xCE: // into
			ip++;
			if (overflowFlag)
				interrupt(4);
			break;
		case 0xcf: // IRET
			ip=getmemuint16(ss,sp);
			sp+=2;
			cs=getmemuint16(ss,sp);
			sp+=2;
			testuint16=getmemuint16(ss,sp);
			sp+=2;
			carryFlag=testuint16&1;
			parityFlag=(testuint16>>2)&1;
			auxFlag=(testuint16>>4)&1;
			zeroFlag=(testuint16>>6)&1;
			signFlag=(testuint16>>7)&1;
			trapFlag=(testuint16>>8)&1;
			interruptFlag=(testuint16>>9)&1;
			directionFlag=(testuint16>>10)&1;
			overflowFlag=(testuint16>>11)&1;
			break;
		case 0xd0: // group #2 Eb,1
			modrmbyte=getmemuint8(cs,(ip)+1);
			earegmode=EA_REGS8;
			regmode=REG_REGS8;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // rol effective address byte, 1, may be able to combine first line into second
				if (effectiveAddressIsRegister) {
					testuint8=getabsolutememuint8(effectiveAddressPointer);
				} else {
					testuint8=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				}
				result8=(testuint8<<1)|(testuint8>>7);
				carryFlag=result8&1;
				SETEARESULT8
				overflowFlag=(result8>>7)!=carryFlag; 
				break;
			case 0x8: // ror effective address byte, 1
				testuint8=1;
				testuint81=getabsolutememuint8(effectiveAddressPointer);
				result8=(testuint81>>testuint8)|(testuint81<<(8-testuint8));
				SETEARESULT8
				carryFlag=testuint81&1;
				overflowFlag=(result8>>7)^((result8>>6)&1);
				break;
			case 0x10: // rcl effective address byte, 1
				testuint8=1;
				// do not change this to use macro
				if (effectiveAddressIsRegister) {
					testuint16=getabsolutememuint8(effectiveAddressPointer);
				} else {
					testuint16=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				}
				testuint16|=(uint16)carryFlag<<8;
				result16=(testuint16<<testuint8)|(testuint16>>(9-testuint8));
				result8=(uint8)result16;
				carryFlag=(result16>>8)&1;
				if (effectiveAddressIsRegister) {
					setabsolutememuint8(effectiveAddressPointer, result8);
				} else {
					setmemuint8(effectiveAddressSegment,effectiveAddressOffset,result8);
				}
				if (testuint8==1) {
					overflowFlag=(testuint8!=carryFlag);
				}
				break;
			case 0x18: // rcr effective address byte, 1
				testuint8=1;
				if (effectiveAddressIsRegister) {
					result8=getabsolutememuint8(effectiveAddressPointer);
				} else {
					result8=getmemuint8(effectiveAddressSegment,effectiveAddressOffset);
				}
				while (testuint8) {
					testuint81=carryFlag;
					carryFlag=result8&1;
					result8>>=1;
					result8|=testuint81<<7;
					testuint8--;
				}
				if (testuint8==1) {
					if ((result8>>7)==((result8>>6)&1)) {
						overflowFlag=0;
					} else {
						overflowFlag=1;
					}
				}
				SETEARESULT8
				break;
			case 0x20:
			case 0x30: // shl effective address byte, 1
				olddestvalue8=getabsolutememuint8(effectiveAddressPointer);
				carryFlag=olddestvalue8>>7; // carryFlag is last number shifted out
				result8=olddestvalue8<<1;
				SETEARESULT8
				signFlag=result8>>7;
				overflowFlag=signFlag!=carryFlag; // applies only to shifts of 1
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				break;
			case 0x28: // shr effective address byte, 1
				testuint8=1;
				olddestvalue8=getabsolutememuint8(effectiveAddressPointer);
				carryFlag=olddestvalue8&1;
				result8=olddestvalue8>>1;
				SETEARESULT8
				overflowFlag=result8>>7;
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				break;
			case 0x38: // sar effective address byte, 1
				GETEAOLDDESTVALUE8
				carryFlag=olddestvalue8&1;
				result8=(sint8)olddestvalue8>>1;
				SETEARESULT8
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				overflowFlag=0; // overflow flag set for single shifts only
				break;
			}
			ip+=instructionSize;
			break;
		case 0xd1:
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // rol effective address word, 1
				if (effectiveAddressIsRegister) {
					testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				} else {
					testuint16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				result16=(testuint16<<1)|(testuint16>>15);
				carryFlag=result16&1;
				SETEARESULT16
				overflowFlag=(result16>>15)!=carryFlag;
				break;
			case 0x8: // ror effective address word, 1
				testuint8=1;
				if (effectiveAddressIsRegister) {
					testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				} else {
					testuint16=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				result16=(testuint16>>testuint8)|(testuint16<<(16-testuint8));
				SETEARESULT16
				carryFlag=(uint8)testuint16&1;
				overflowFlag=(result16>>15)^((result16>>14)&1);
				break;
			case 0x10: // rcl effective address word, 1
				testuint8=1;
				if (effectiveAddressIsRegister) {
					testuint32=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				} else {
					testuint32=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				testuint32|=(uint32)carryFlag<<16;
				result32=(testuint32<<testuint8)|(testuint32>>(17-testuint8));
				result16=(uint16)result32;
				carryFlag=(result32>>16)&1;
				SETEARESULT16
				if (testuint8==1) {
					overflowFlag=(testuint8!=carryFlag);
				}
				break;
			case 0x18: // rcr effective address word, 1
				testuint8=1;
				if (effectiveAddressIsRegister) {
					testuint32=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				} else {
					testuint32=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				}
				testuint32|=(uint32)carryFlag<<16;
				result32=(testuint32>>testuint8)|(testuint32<<(17-testuint8));
				result16=(uint16)result32;
				carryFlag=(result32>>16)&1;
				SETEARESULT16
				if ((result8>>7)==((result8>>6)&1)) {
					overflowFlag=0;
				} else {
					overflowFlag=1;
				}
				break;
			case 0x20:
			case 0x30: // shl effective address word, 1
				GETEAOLDDESTVALUE16
				carryFlag=(olddestvalue16>>15)&1; // carryFlag is last number shifted out
				result16=olddestvalue16<<1;
				SETEARESULT16
				signFlag=result16>>15;
				overflowFlag=signFlag!=carryFlag; // applies only to shifts of 1
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				break;
			case 0x28: // shr effective address word, 1
				testuint8=1;
				GETEAOLDDESTVALUE16
				carryFlag=(olddestvalue16>>(testuint8-1))&1;
				result16=olddestvalue16>>testuint8;
				SETEARESULT16
				overflowFlag=result16>>15;
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				break;
			case 0x38: // sar effective address word, 1
				GETEAOLDDESTVALUE16
				carryFlag=olddestvalue16&1;
				result16=(sint16)olddestvalue16>>1;
				SETEARESULT16
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				overflowFlag=0;
				break;
			}
			ip+=instructionSize;
			break;
		case 0xd2:
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS8;
			regmode=REG_REGS8;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // rol effective address byte, cl
				testuint8=cl;
				result8=(getabsolutememuint8(effectiveAddressPointer)<<testuint8)|(getabsolutememuint8(effectiveAddressPointer)>>(8-testuint8));
				carryFlag=result8&1;
				setabsolutememuint8(effectiveAddressPointer, result8);
				overflowFlag=(testuint8!=carryFlag);
				break;
			case 0x8: // ror effective address byte, cl
				testuint8=cl;
				testuint81=getabsolutememuint8(effectiveAddressPointer);
				result8=(testuint81>>testuint8)|(testuint81<<(8-testuint8));
				setabsolutememuint8(effectiveAddressPointer, result8);
				overflowFlag=(result8>>7)^((result8>>6)&1);
				carryFlag=testuint81&1;
				break;
			case 0x10: // rcl effective address byte, cl
				testuint8=cl;
				testuint16=getabsolutememuint8(effectiveAddressPointer);
				testuint16|=(uint16)carryFlag<<8;
				result16=(testuint16<<testuint8)|(testuint16>>(9-testuint8));
				result8=(uint8)result16;
				carryFlag=(result16>>8)&1;
				setabsolutememuint8(effectiveAddressPointer, result8);
				if (testuint8==1) {
					overflowFlag=(testuint8!=carryFlag);
				}
				break;
			case 0x18: // rcr effective address byte, cl
				testuint8=cl;
				result8=getabsolutememuint8(effectiveAddressPointer);
				while (testuint8) {
					testuint81=carryFlag;
					carryFlag=result8&1;
					result8>>=1;
					result8|=testuint81<<7;
					testuint8--;
				}
				if (testuint8==1) {
					if ((result8>>7)==((result8>>6)&1)) {
						overflowFlag=0;
					} else {
						overflowFlag=1;
					}
				}
				setabsolutememuint8(effectiveAddressPointer, result8);
				break;
			case 0x20:
			case 0x30: // shl effective address byte, cl
				GETEAOLDDESTVALUE8
				carryFlag=(olddestvalue8>>(8-cl))&1; // carryFlag is last number shifted out
				result8=olddestvalue8<<cl;
				SETEARESULT8
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				break;
			case 0x28: // shr effective address byte, cl
				GETEAOLDDESTVALUE8
				carryFlag=(olddestvalue8>>(cl-1))&1;
				result8=olddestvalue8>>cl;
				SETEARESULT8
				overflowFlag=result8>>7;
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;		
				break;
			case 0x38: // sar effective address byte, cl
				GETEAOLDDESTVALUE8
				carryFlag=(olddestvalue8>>(cl-1))&1;
				result8=(sint8)olddestvalue8>>cl;
				SETEARESULT8
				zeroFlag=result8==0;
				parityFlag=paritylookup[result8];
				signFlag=result8>>7;
				overflowFlag=0;
				break;
			}
			ip+=instructionSize;
			break;
		case 0xd3:
			modrmbyte=getmemuint8(cs,(ip)+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0:
				// rol effective address word, cl
				testuint8=cl;
				testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				result16=(testuint16<<testuint8)|(testuint16>>(16-testuint8));
				carryFlag=result16&1;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				overflowFlag=(testuint8!=carryFlag);
				break;
			case 0x8:
				// ror effective address word, cl
				testuint8=cl;
				testuint16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				result16=(testuint16>>testuint8)|(testuint16<<(16-testuint8));
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				overflowFlag=(result16>>15)^((result16>>14)&1);
				carryFlag=(uint8)testuint16&1;
				break;
			case 0x10:
				// rcl effective address word, cl
				testuint8=cl;
				testuint32=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				testuint32|=(uint32)carryFlag<<16;
				result32=(testuint32<<testuint8)|(testuint32>>(17-testuint8));
				result16=(uint16)result32;
				carryFlag=(result32>>16)&1;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				if (testuint8==1) {
					overflowFlag=(testuint8!=carryFlag);
				}
				break;
			case 0x18:
				// rcr effective address word, cl
				testuint8=cl;
				testuint32=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				testuint32|=(uint32)carryFlag<<16;
				result32=(testuint32>>testuint8)|(testuint32<<(17-testuint8));
				result16=(uint16)result32;
				carryFlag=(result32>>16)&1;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				if (testuint8==1) {
					if ((result8>>7)==((result8>>6)&1)) {
						overflowFlag=0;
					} else {
						overflowFlag=1;
					}
				}
				break;
			case 0x20:
			case 0x30:
				// shl effective address word, cl
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				carryFlag=(olddestvalue16>>(16-cl))&1; // carryFlag is last number shifted out
				result16=olddestvalue16<<cl;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				signFlag=result16>>15;
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				break;
			case 0x28: // shr effective address word, cl
				testuint8=cl;
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				carryFlag=(olddestvalue16>>(cl-1))&1;
				result16=olddestvalue16>>cl;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				break;
			case 0x38: // sar effective address word, cl
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				carryFlag=(olddestvalue16>>(cl-1))&1;
				result16=(sint16)olddestvalue16>>cl;
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				zeroFlag=result16==0;
				parityFlag=paritylookup[result16&255];
				signFlag=result16>>15;
				overflowFlag=0;
				break;
			}
			ip+=instructionSize;
			break;
		case 0xd4: // AAM
			ADVANCETIME(83);
			ah=al/getmemuint8(cs,ip+1);
			al=al%getmemuint8(cs,ip+1);
			zeroFlag=al==0;
			signFlag=al>>7;
			parityFlag=paritylookup[al];
			ip+=2;
			break;
		case 0xd5: // AAD
			ADVANCETIME(60);
			al=ah*getmemuint8(cs,ip+1)+al;
			ah=0;
			zeroFlag=al==0;
			signFlag=al>>7;
			parityFlag=paritylookup[al];
			ip+=2;
			break;
		case 0xD6: // SALC
			if (carryFlag)
				al=0xff;
			else
				al=0;
			ip++;
			break;
		case 0xd7: // XLAT
			al=getmemuint8(*activeSegment,bx+al);
			ip++;
			break;
		case 0xd8:
		case 0xd9:
		case 0xda:
		case 0xdb:
		case 0xdc:
		case 0xdd:
		case 0xde:
		case 0xdf: // fpu operands
			ip++;
			interrupt(7);
			break;
		case 0xe0: // LOOPNE/LOOPNZ
			cx--;
			if ((cx!=0) && (zeroFlag==0)) {
				ip+=(sint8)getmemuint8(cs,ip+1)+2;
			} else {
				ip+=2;
			}
			break;
		case 0xe1: // LOOPE/LOOPZ
			cx--;
			if ((cx!=0) && (zeroFlag==1)) {
				ip+=(sint8)getmemuint8(cs,ip+1)+2;
			} else {
				ip+=2;
			}
			break;
		case 0xe2: // LOOP Jb
			cx--;
			if (cx!=0) {
				ip+=(sint8)getmemuint8(cs,ip+1)+2;
			} else {
				ip+=2;
			}
			break;
		case 0xe3: // JCXZ
			if (cx==0) {
				ip+=(sint8)getmemuint8(cs,ip+1)+2;
			} else {
				ip+=2;
			}
			break;
		case 0xe4: // IN AL,lb
			catchPortIn8(getmemuint8(cs,ip+1),&al);
			ip+=2;
			break;
		case 0xe5: // IN AX,lw
			catchPortIn8(getmemuint8(cs,ip+1),&al);
			catchPortIn8(getmemuint8(cs,ip+1),&ah);
			/* JS */
			ip+=3;
			break;
		case 0xe6: // OUT AL,lb
			catchPortOut8(getmemuint8(cs,ip+1),al);
			ip+=2;
			break;
		case 0xe7: // OUT AX,lw
			catchPortOut8(getmemuint8(cs,ip+1),al);
			catchPortOut8(getmemuint8(cs,ip+1),ah);
			ip+=3;
			break;
		case 0xe8: // CALL Jv
			sp-=2;
			setmemuint16(ss,sp,ip+3);
			ip+=getmemuint16(cs,ip+1)+3;
			break;
		case 0xe9: // relative JMP (16)
			ip+=getmemuint16(cs,ip+1)+3;
			break;
		case 0xea: // absolute JMP
			testuint16=cs;
			testuint161=ip;
			ip=getmemuint16(testuint16,testuint161+1);
			cs=getmemuint16(testuint16,testuint161+3);
			break;
		case 0xeb:
			ip+=(sint8)getmemuint8(cs,(ip)+1)+2;
			break;
		case 0xec:
			catchPortIn8(dx,&al);
			ip++;
			break;
		case 0xed:
			catchPortIn8(dx,&al);
			catchPortIn8(dx+1,&ah);
			ip++;
			break;
		case 0xee: // OUT DX,AL
			catchPortOut8(dx,al);
			ip++;
			break;
		case 0xef: // OUT DX,AX
			catchPortOut8(dx, al);
			catchPortOut8(dx+1, ah);
			ip++;
			break;
		case 0xf0: // LOCK
			ip++;
			break;
		case 0xf1: //INT1
			ip++;
			interrupt(1);
			break;
		case 0xf4: // hlt
			ip++;
			break;
		case 0xf5: // cmc
			carryFlag=(carryFlag==0 ? 1 : 0);
			ip++;
			break;
		case 0xf6:
			// group 3 Eb
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS8;
			regmode=REG_REGS8;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0:
			case 0x8: // test
				result8=getabsolutememuint8(effectiveAddressPointer)&getmemuint8(cs,ip+instructionSize);
				SETFLAGS_BOOLEAN_8
				ip+=instructionSize+1;
				break;
			case 0x10: // not
				setabsolutememuint8(effectiveAddressPointer, ~getabsolutememuint8(effectiveAddressPointer));
				ip+=instructionSize;
				break;
			case 0x18: // neg
				testuint8=getabsolutememuint8(effectiveAddressPointer);
				carryFlag=(testuint8!=0);
				olddestvalue8=getabsolutememuint8(effectiveAddressPointer);
				result8=-testuint8;
				setabsolutememuint8(effectiveAddressPointer, result8);
				signFlag=result8>>7;
				zeroFlag=result8==0;
				overflowFlag=((result8^olddestvalue8)&0x80);
				parityFlag=paritylookup[result8];
				ip+=instructionSize;
				break;
			case 0x20: // mul
				ax=al*getabsolutememuint8(effectiveAddressPointer);
				carryFlag=overflowFlag=(ah>0);
				ip+=instructionSize;
				break;
			case 0x28:  // imul
				ax=(sint8)al*(*(sint8 *)effectiveAddressPointer);
				if (ah==0) { // flag settings not right here, see i386
					carryFlag=0;
					overflowFlag=0;
				} else {
					carryFlag=1;
					overflowFlag=1;
				}
				ip+=instructionSize;
				break;
			case 0x30: // div
				testuint16=ax;
				if (getabsolutememuint8(effectiveAddressPointer)==0) {
					ip+=instructionSize;
					interrupt(0);
				} else {
					testuint161=testuint16 / getabsolutememuint8(effectiveAddressPointer);
					if (testuint161>255) {
						ip+=instructionSize;
						interrupt(0);
					} else {
						ah=testuint16 % getabsolutememuint8(effectiveAddressPointer);
						al=(uint8)testuint161;
						ip+=instructionSize;
					}
				}
				break;
			case 0x38: // idiv
				if (getabsolutememuint8(effectiveAddressPointer)==0) {
					ip+=instructionSize;
					interrupt(0);
				} else {
					testsint16 = ax;
					testsint161 = testsint16 / *(sint8 *)effectiveAddressPointer;
					if (testsint161 > 127 || testsint161 < -128) {
						ip+=instructionSize;
						interrupt(0);
					} else {
						ah = testsint16 % *(sint8 *)effectiveAddressPointer;
						al = (uint8)testsint161;
						ip+=instructionSize;
					}
				}
				break;
			}
			break;
		case 0xf7:
			// group 3 Ew
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0:
			case 0x8: // test 
				result16=getabsolutememuint16((uint16 *)effectiveAddressPointer)&getmemuint16(cs,ip+instructionSize);
				SETFLAGS_BOOLEAN_16
				ip+=instructionSize+2;
				break;
			case 0x10: // not 
				setabsolutememuint16((uint16 *)effectiveAddressPointer,~getabsolutememuint16((uint16 *)effectiveAddressPointer));
				ip+=instructionSize;
				break;
			case 0x18: // neg
				carryFlag=(getabsolutememuint16((uint16 *)effectiveAddressPointer)!=0);
				olddestvalue16=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				result16=-getabsolutememuint16((uint16 *)effectiveAddressPointer);
				setabsolutememuint16((uint16 *)effectiveAddressPointer,result16);
				signFlag=result16>>15;
				zeroFlag=result16==0;
				overflowFlag=((result16^olddestvalue16)&0x8000);
				parityFlag=paritylookup[result16&255];
				ip+=instructionSize;
				break;
			case 0x20: // mul
				testuint32=ax*(getabsolutememuint16((uint16 *)effectiveAddressPointer));
				ax=(uint16)testuint32&0xffff;
				dx=(uint16)(testuint32>>16);
				overflowFlag=carryFlag=(dx>0);
				ip+=instructionSize;
				break;
			case 0x28:  // imul
				testuint32=(sint16)ax*(sint16)getabsolutememuint16((uint16 *)effectiveAddressPointer);
				ax=(uint16)testuint32;
				dx=(uint16)(testuint32>>16);
				if (((sint32)testuint32>32767) || ((sint32)testuint32<-32768)) { // flag settings probably not right here, see i386
					overflowFlag=carryFlag=0;
				} else {
					overflowFlag=carryFlag=1;
				}
				ip+=instructionSize;
				break;
			case 0x30: // div
				if (getabsolutememuint16((uint16 *)effectiveAddressPointer)==0) {
					ip+=instructionSize;
					interrupt(0);
				} else {
					testuint32=(dx<<16)|ax;
					testuint321=testuint32/ getabsolutememuint16((uint16 *)effectiveAddressPointer);
					if (testuint321>65535) {
						ip+=instructionSize;
						interrupt(0); // this interrupt should be triggered after the instruction
					} else {
						ax=testuint321;
						dx=testuint32% getabsolutememuint16((uint16 *)effectiveAddressPointer);
						ip+=instructionSize;
					}
				}
				break;
			case 0x38: // idiv
				if (getabsolutememuint16((uint16 *)effectiveAddressPointer)==0) {
					ip+=instructionSize;
					interrupt(0);
				} else {
					testsint32 = (dx<<16)|ax;
					testsint321 = testsint32 / (sint16)getabsolutememuint16((uint16*)effectiveAddressPointer);
					if ( testsint321>32767 || testsint321<-32768 ) {
						ip+=instructionSize;
						interrupt(0);
					} else {
						ax = (uint16)testsint321;
						dx = testsint32 % (sint16)getabsolutememuint16((uint16*)effectiveAddressPointer);
						ip+=instructionSize;
					}
				}
				break;
			}
			break;
		case 0xf8: // clc
			carryFlag=0;
			ip++;
			break;
		case 0xf9: // stc
			carryFlag=1;
			ip++;
			break;
		case 0xfa: // cli
			interruptFlag=0;
			ip++;
			break;
		case 0xfb: // sti
			interruptFlag=1;
			ip++;
			break;
		case 0xfc: // cld
			directionFlag=0;
			directionModifier8=1;
			directionModifier16=2;
			ip++;
			break;
		case 0xfd: // std
			directionFlag=1;
			directionModifier8=-1;
			directionModifier16=-2;
			ip++;
			break;
		case 0xfe:
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS8;
			regmode=REG_REGS8;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0: // inc
				GETEAOLDDESTVALUE8
				result8=(getabsolutememuint8(effectiveAddressPointer))+1;
				SETEARESULT8
				signFlag=result8>>7;
				zeroFlag=result8==0;
				overflowFlag=((result8^olddestvalue8)&0x80);
				auxFlag=((result8>=((olddestvalue8&240)+16))&&(result8<olddestvalue8));
				parityFlag=paritylookup[result8];
				ip+=instructionSize;
				break;
			case 0x8: // dec
				GETEAOLDDESTVALUE8
				oper8=1;
				result8=(getabsolutememuint8(effectiveAddressPointer))-oper8;
				SETEARESULT8
				signFlag=result8>>7;
				zeroFlag=result8==0;
				overflowFlag=!!((result8^olddestvalue8)&(oper8^olddestvalue8)&0x80);
				auxFlag=(result8<(olddestvalue8&240))&&(result8>olddestvalue8);
				parityFlag=paritylookup[result8];
				ip+=instructionSize;
				break;
			case 0x38:
				nativeInterrupt(getmemuint8(cs,ip+2));
				ip+=3;
				break;
			}
			break;
	
		case 0xff:
			modrmbyte=getmemuint8(cs,ip+1);
			earegmode=EA_REGS16;
			regmode=REG_REGS16;
			decodegroupmodrmbyte();
			switch (modrmbyte&0x38) {
			case 0:	// inc
				GETEAOLDDESTVALUE16
				oper16=1;
				result16=olddestvalue16+oper16;
				SETEARESULT16
				signFlag=result16>>15;
				zeroFlag=result16==0;
				overflowFlag=!!((result16^oper16)&(result16^olddestvalue16)&0x8000);	
				auxFlag=((result16>=((olddestvalue16&240)+16))&&(result16<olddestvalue16));
				parityFlag=paritylookup[result16&255];
				ip+=instructionSize;
				break;
			case 0x8: // dec
				GETEAOLDDESTVALUE16
				oper16=1;
				result16=olddestvalue16-oper16;
				SETEARESULT16
				signFlag=result16>>15;
				zeroFlag=result16==0;
				overflowFlag=!!((result16^olddestvalue16)&(oper16^olddestvalue16)&0x8000);
				auxFlag=(result16<(olddestvalue16&240))&&(result16>olddestvalue16);
				parityFlag=paritylookup[result16&255];
				ip+=instructionSize;
				break;
			case 0x10: // call Ev
				sp-=2;
				setmemuint16(ss,sp,ip+instructionSize);
				ip=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				break;
			case 0x18: // call Mp
				sp-=2;
				setmemuint16(ss,sp,cs);
				sp-=2;
				setmemuint16(ss,sp,ip+instructionSize);
				ip=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				cs=getmemuint16(effectiveAddressSegment,effectiveAddressOffset+2);
				break;
			case 0x20: // jmp Ev
				ip=getabsolutememuint16((uint16 *)effectiveAddressPointer);
				break;
			case 0x28:  // jmp Mp
				ip=getmemuint16(effectiveAddressSegment,effectiveAddressOffset);
				cs=getmemuint16(effectiveAddressSegment,effectiveAddressOffset+2);
				break;
			case 0x30: // push Ev
				sp-=2;
				setmemuint16(ss,sp,getabsolutememuint16((uint16 *)effectiveAddressPointer));
				ip+=instructionSize;
				break;
			case 0x38:
				interrupt(6);
	
			}
			break;
		default:
			interrupt(6);
		}

	}
}
