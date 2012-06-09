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

#include"disasm.h"

CDisasm86::CDisasm86(Cpu86 *cpu) {
	FILE *eaImage;
	FILE *infile;
	int bytesRead;
	int current=0;
	int nextIsPtr=1;
	int currentTableEntry=0;
	char line[100];
	int lcount=0;
	int gcount=0;
	int opcodeStringsOffset=0;
	int opcodeStringLength;
	int groupOpcodeStringsOffset=0;
	int groupOpcodeStringLength;

	eaStrings=new char[500];
	eatable=new char*[100];

	opcodeStrings=new char[10000];
	opcodeTable=new char*[256];

	groupOpcodeStrings=new char[10000];
	groupOpcodeTable=new char*[256];

	stringBuffer=new char[100];

	this->cpu=cpu;

	printf("loading effective address table...\n");
	eaImage=fopen("eatable.txt","rb");
	bytesRead=fread(eaStrings,1,512,eaImage);
	fclose(eaImage);

	if((infile = fopen("opcodetable.txt", "r")) == NULL) {
		printf("Error Opening Opcodes File.\n");
	}
	while( fgets(line, sizeof(line), infile) != NULL ) {
		line[strlen(line)-1]=0;
		if (line[0]=='-') {
			groupOpcodeStringLength=strlen(line)-1;
			strcpy(&groupOpcodeStrings[groupOpcodeStringsOffset],&line[1]);
			groupOpcodeTable[gcount]=&groupOpcodeStrings[groupOpcodeStringsOffset];
			groupOpcodeStringsOffset+=groupOpcodeStringLength+1;
			gcount++;

		} else {
			opcodeStringLength=strlen(line);
			strcpy(&opcodeStrings[opcodeStringsOffset],line);
			opcodeTable[lcount]=&opcodeStrings[opcodeStringsOffset];
			opcodeStringsOffset+=opcodeStringLength+1;
			lcount++;
		}

	}
	fclose(infile);

	while (current<bytesRead) {
		if (nextIsPtr) {
			eatable[currentTableEntry]=&eaStrings[current];
			currentTableEntry++;
		}
		if (eaStrings[current]==',') {
			eaStrings[current]=0;
			nextIsPtr=true;
		} else {
			nextIsPtr=false;
		}
		current++;
	}

	eaStringOutput=new char[100];
	debugScreenBuffer=new uint8[80*25*2];
	debugScreen=new CVideoDriverPalette3h(debugScreenBuffer);
	debugScreen->disablePageSwitching();

	for (int index=0; index<2000; index++) {
		debugScreenBuffer[index*2]=0;
		debugScreenBuffer[index*2+1]=0x0f;
	}

	setAreaColour(1,12,22,23,6);
	setAreaColour(24,12,57,23,10);
	setAreaColour(61,12,77,17,11);
	setAreaColour(61,18,77,23,13);
	setAreaColour(1,3,57,11,7);
	setAreaColour(61,3,77,10,14);
}

CDisasm86::~CDisasm86() {
	delete debugScreen;
	delete []debugScreenBuffer;
	delete []eaStringOutput;
	delete []stringBuffer;
	delete []groupOpcodeTable;
	delete []groupOpcodeStrings;
	delete []opcodeTable;
	delete []opcodeStrings;
	delete []eatable;
	delete []eaStrings;
}

void CDisasm86::decodemodrmbyte() {

	modrmbyte=cpu->getmemuint8(cs,ip+1);

	regOffset=(modrmbyte>>3)&7;
	eaStringInput=eatable[((modrmbyte>>6)<<3)+(modrmbyte&7)];

	uint16 sword;
	sint8 sbyte;
	switch(modrmbyte>>6) {
		case 0:
			if ((modrmbyte&7)==6) {
				sword=cpu->getmemuint16(cs,ip+2);
				if (explicitEaSize) {
					if (eaSize==16) {
						sprintf(eaStringOutput,eaStringInput,"word ptr ",sword);
					} else {
						sprintf(eaStringOutput,eaStringInput,"byte ptr ",sword);
					}
				} else {
					sprintf(eaStringOutput,eaStringInput,"",sword);
				}
				instructionSize=4;
			} else {
				instructionSize=2;
				if (explicitEaSize) {
					if (eaSize==16) {
						sprintf(eaStringOutput,eaStringInput,"word ptr ");
					} else {
						sprintf(eaStringOutput,eaStringInput,"byte ptr ");
					}
				} else {
					sprintf(eaStringOutput,eaStringInput,"");
				}
			}
			break;
		case 1:
			sbyte=cpu->getmemuint8(cs,ip+2);
			instructionSize=3;

			if (explicitEaSize) {
				if (eaSize==16) {
					if (sbyte>=0) {
						sprintf(eaStringOutput,eaStringInput,"word ptr ","+",sbyte);
					} else {
						sbyte=-sbyte;
						sprintf(eaStringOutput,eaStringInput,"word ptr ","-",sbyte);
					}
				} else {
					if (sbyte>=0) {
						sprintf(eaStringOutput,eaStringInput,"byte ptr ","+",sbyte);
					} else {
						sbyte=-sbyte;
						sprintf(eaStringOutput,eaStringInput,"byte ptr ","-",sbyte);
					}

				}
			} else {
				if (sbyte>=0) {
					sprintf(eaStringOutput,eaStringInput,"","+",sbyte);
				} else {
					sbyte=-sbyte;
					sprintf(eaStringOutput,eaStringInput,"","-",sbyte);
				}

			}


			break;
		case 2:
			sword=cpu->getmemuint16(cs,ip+2);
			instructionSize=4;

			if (explicitEaSize) {
				if (eaSize==16) {
					sprintf(eaStringOutput,eaStringInput,"word ptr ",sword);
				} else {
					sprintf(eaStringOutput,eaStringInput,"byte ptr ",sword);
				}
			} else {
				sprintf(eaStringOutput,eaStringInput,"",sword);
			}
			break;
		case 3:
			instructionSize=2;
			if (eaSize==16) {
				eaStringInput=eatable[((modrmbyte>>6)<<3)+(modrmbyte&7)+8];
			}
			strcpy(eaStringOutput,eaStringInput);
			break;
	}
}

void CDisasm86::advanceIP() {
	ip+=instructionSize;
}

void CDisasm86::matchCSIP() {
	cs=cpu->cs;
	ip=cpu->ip;
}

void CDisasm86::render() {
	paint();
	debugScreen->render();
}

void CDisasm86::paint() {

	static int index,index2;

	for (int index=0; index<2000; index++) {
		debugScreenBuffer[index*2]=0;
	}

	sprintf(stringBuffer,"Legacy VM Monitor ----------------- F1 SingleStep -- F12 ReturnToEmulation --");
	writeString(1,1);

	sprintf(stringBuffer,"ax=");
	writeString(61,12);
	generateHex16(cpu->ax);
	writeString(64,12);

	sprintf(stringBuffer,"bx=");
	writeString(61,13);
	generateHex16(cpu->bx);
	writeString(64,13);

	sprintf(stringBuffer,"cx=");
	writeString(61,14);
	generateHex16(cpu->cx);
	writeString(64,14);

	sprintf(stringBuffer,"dx=");
	writeString(61,15);
	generateHex16(cpu->dx);
	writeString(64,15);

	sprintf(stringBuffer,"si=");
	writeString(61,16);
	generateHex16(cpu->si);
	writeString(64,16);

	sprintf(stringBuffer,"di=");
	writeString(61,17);
	generateHex16(cpu->di);
	writeString(64,17);

	sprintf(stringBuffer,"cs=");
	writeString(71,12);
	generateHex16(cpu->cs);
	writeString(74,12);

	sprintf(stringBuffer,"ds=");
	writeString(71,13);
	generateHex16(cpu->ds);
	writeString(74,13);

	sprintf(stringBuffer,"es=");
	writeString(71,14);
	generateHex16(cpu->es);
	writeString(74,14);

	sprintf(stringBuffer,"ss=");
	writeString(71,15);
	generateHex16(cpu->ss);
	writeString(74,15);

	sprintf(stringBuffer,"sp=");
	writeString(71,16);
	generateHex16(cpu->sp);
	writeString(74,16);

	sprintf(stringBuffer,"bp=");
	writeString(71,17);
	generateHex16(cpu->bp);
	writeString(74,17);

	if (cpu->carryFlag) {
		sprintf(stringBuffer,"CF");
		writeString(64,20);
	}

	if (cpu->parityFlag) {
		sprintf(stringBuffer,"PF");
		writeString(67,20);
	}

	if (cpu->auxFlag) {
		sprintf(stringBuffer,"AF");
		writeString(70,20);
	}

	if (cpu->zeroFlag) {
		sprintf(stringBuffer,"ZF");
		writeString(73,20);
	}

	if (cpu->signFlag) {
		sprintf(stringBuffer,"SF");
		writeString(64,22);
	}

	if (cpu->trapFlag) {
		sprintf(stringBuffer,"TF");
		writeString(67,22);
	}

	if (cpu->directionFlag) {
		sprintf(stringBuffer,"DF");
		writeString(70,22);
	}

	if (cpu->overflowFlag) {
		sprintf(stringBuffer,"OF");
		writeString(73,22);
	}

	matchCSIP();
	for (index=0; index<12; index++) {

		decodeInstruction();
		writeString(24,index+12);

		for (index2=0; index2<instructionSize; index2++) {
			generateHex8(cpu->getmemuint8(cs,ip+index2));
			writeString(11+(index2*2),index+12);
		}

		generateHex16(cs);
		writeString(1,index+12);
		sprintf(stringBuffer,":");
		writeString(5,index+12);
		generateHex16(ip);
		writeString(6,index+12);

		advanceIP();

	}

	for (index=0; index<8; index++) {
		generateHex16(index);
		writeString(1,index+3);
		sprintf(stringBuffer,":0000");
		writeString(5,index+3);
		for (index2=0; index2<16; index2++) {
			generateHex8(cpu->getmemuint8(index+3,index2));
			writeString(11+(index2*3),index+3);
			
		}
	}

	//sprintf(stringBuffer,"head: %d tail: %d",cpu->getmemuint16(0x40,0x1a),cpu->getmemuint16(0x40,0x1c));
	//writeString(1,11);

	for (index=0; index<8; index++) {
		for (index2=0; index2<16; index2++) {
			debugScreenBuffer[((index+3)*160)+((index2+61)*2)]=cpu->getmemuint8(index+3,index2);
		}
	}

}

void CDisasm86::generateHex16(uint16 val) {
	if (val<0x10) {
		sprintf(stringBuffer,"000%x",val);
	} else {
		if (val<0x100) {
			sprintf(stringBuffer,"00%x",val);
		} else {
			if (val<0x1000) {
				sprintf(stringBuffer,"0%x",val);
			} else {
				sprintf(stringBuffer,"%x",val);
			}
		}
	}
}

void CDisasm86::generateHex8(uint8 val) {
	if (val<0x10) {
		sprintf(stringBuffer,"0%x",val);
	} else {
		sprintf(stringBuffer,"%x",val);
	}
}


void CDisasm86::writeString(int x,int y) {
	uint8 *in;
	uint8 *out;

	out=&debugScreenBuffer[(y*2*80)+(x*2)];
	in=(uint8 *)stringBuffer;

	while (*in>0) {
		*out=*in;
		in++;
		out+=2;

	}
}

void CDisasm86::setAreaColour(int x1,int y1,int x2,int y2,uint8 col) {
	int x,y;
	for (y=y1; y<=y2; y++) {
		for (x=x1; x<=x2; x++) {
			debugScreenBuffer[(y*160)+(x*2)+1]=col;
		}
	}
}

void CDisasm86::decodeInstruction() {

	opcode=cpu->getmemuint8(cs,ip);
	switch(opcode) {

	// OPCODE Eb,Gb
	case 0x0:
	case 0x10:
	case 0x20:
	case 0x30:
	case 0x84:
	case 0x86:
	case 0x08:
	case 0x18:
	case 0x28:
	case 0x38:
	case 0x88:
		eaSize=8;
		explicitEaSize=false;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eaStringOutput,eatable[regOffset+24]);
	break;

	// OPCODE Gb,Eb
	case 0x02:
	case 0x12:
	case 0x22:
	case 0x32:
	case 0x0A:
	case 0x1A:
	case 0x2A:
	case 0x3A:
	case 0x8A:
		eaSize=8;
		explicitEaSize=false;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eatable[regOffset+24],eaStringOutput);
	break;

	// OPCODE Ev,Gv
	case 0x01:
	case 0x11:
	case 0x21:
	case 0x31:
	case 0x85:
	case 0x87:
	case 0x09:
	case 0x19:
	case 0x29:
	case 0x39:
	case 0x89:
		eaSize=16;
		explicitEaSize=false;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eaStringOutput,eatable[regOffset+32]);
	break;

	// OPCODE Gv,Ev
	case 0x03:
	case 0x13:
	case 0x23:
	case 0x33:
	case 0x0B:
	case 0x1B:
	case 0x2B:
	case 0x3B:
	case 0x8B:
	case 0xc4:
	case 0xc5:
	case 0x8d:
		eaSize=16;
		explicitEaSize=false;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eatable[regOffset+32],eaStringOutput);
	break;

	// OPCODE Ib
	case 0x04:
	case 0x14:
	case 0x24:
	case 0x34:
	case 0xE4:
	case 0x0C:
	case 0x1C:
	case 0x2C:
	case 0x3C:
	case 0xA8:
	case 0xb0:
	case 0xb1:
	case 0xb2:
	case 0xb3:
	case 0xb4:
	case 0xb5:
	case 0xb6:
	case 0xb7:
	case 0xd4:
	case 0xd5:
	case 0xe5:
	case 0xe6:
	case 0xe7:
	case 0x6a:
	case 0xcd:
		sprintf(stringBuffer,opcodeTable[opcode],cpu->getmemuint8(cs,ip+1));
		instructionSize=2;
	break;

	// OPCODE Jb
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7a:
	case 0x7b:
	case 0x7c:
	case 0x7d:
	case 0x7e:
	case 0x7f:
	case 0xe0:
	case 0xe1:
	case 0xe2:
	case 0xe3:
	case 0xeb:
		tempsint8=cpu->getmemuint8(cs,ip+1);
		sprintf(stringBuffer,opcodeTable[opcode],ip+tempsint8+2);
		instructionSize=2;
	break;

	// immediate word 
	case 0x05:
	case 0x15:
	case 0x25:
	case 0x35:
	case 0x0d:
	case 0x1d:
	case 0x2d:
	case 0x3d:
	case 0xa9:
	case 0xc2:
	case 0xb8:
	case 0xb9:
	case 0xba:
	case 0xbb:
	case 0xbc:
	case 0xbd:
	case 0xbe:
	case 0xbf:
	case 0xa0:
	case 0xa1:
	case 0xa2:
	case 0xa3:
	case 0x68:
	case 0xca:
		instructionSize=3;
		sprintf(stringBuffer,opcodeTable[opcode],cpu->getmemuint16(cs,ip+1));
	break;

	// CALL Jz, JMP Jz
	case 0xe8:
	case 0xe9:
		instructionSize=3;
		sprintf(stringBuffer,opcodeTable[opcode],(ip+3+cpu->getmemuint16(cs,ip+1))&0xffff);
	break;

	// JMP Ap
	case 0xea:
		instructionSize=5;
		sprintf(stringBuffer,opcodeTable[opcode],cpu->getmemuint16(cs,ip+3),cpu->getmemuint16(cs,ip+1));
	break;

	// IMUL Ev,Gv,Iz
	case 0x69:
		eaSize=16;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eatable[regOffset+32],eaStringOutput,cpu->getmemuint16(cs,ip+instructionSize));
		instructionSize+=2;
	break;

	// IMUL Ev,Gv,Ib
	case 0x6b:
		eaSize=16;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eatable[regOffset+32],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// ENTER Iw,Ib
	case 0xc8:
		sprintf(stringBuffer,opcodeTable[opcode],cpu->getmemuint16(cs,ip+1),cpu->getmemuint8(cs,ip+3));
		instructionSize=4;
	break;

	// Call Ap
	case 0x9a:
		sprintf(stringBuffer,opcodeTable[opcode],cpu->getmemuint16(cs,ip+3),cpu->getmemuint16(cs,ip+1));
		instructionSize=5;
	break;

	// mov Mw,Sw
	case 0x8c:
		eaSize=16;
		explicitEaSize=false;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eaStringOutput,eatable[regOffset+40]);
	break;

	// mov Sw,Mw
	case 0x8e:
		eaSize=16;
		explicitEaSize=false;
		decodemodrmbyte();
		sprintf(stringBuffer,opcodeTable[opcode],eatable[regOffset+40],eaStringOutput);
	break;

	// GROUP OPCODES

	// group #1 Eb,Ib
	case 0x80:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #1 Ev,Iz
	case 0x81:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+8],eaStringOutput,cpu->getmemuint16(cs,ip+instructionSize));
		instructionSize+=2;
	break;

	// group #1 Eb,Ib
	case 0x82:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+16],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #1 Ev,Ib
	case 0x83:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+24],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #10
	case 0x8f:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+32],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #2 Eb,Ib
	case 0xc0:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+40],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #2 Ev,Ib
	case 0xc1:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+48],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #12 Eb,Ib
	case 0xc6:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+56],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
		instructionSize++;
	break;

	// group #12 Ev,Iz
	case 0xc7:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+64],eaStringOutput,cpu->getmemuint16(cs,ip+instructionSize));
		instructionSize+=2;
	break;

	// group #2 Eb,1
	case 0xd0:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+72],eaStringOutput);
	break;

	// group #2 Ev,1
	case 0xd1:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+80],eaStringOutput);
	break;

	// group #2 Eb,cl
	case 0xd2:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+88],eaStringOutput);
	break;

	// group #2 Ev,cl
	case 0xd3:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+96],eaStringOutput);
	break;

	// group #3 Eb
	case 0xf6:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		if (regOffset<2) {
			sprintf(stringBuffer,groupOpcodeTable[regOffset+104],eaStringOutput,cpu->getmemuint8(cs,ip+instructionSize));
			instructionSize+=1;
		} else {
			sprintf(stringBuffer,groupOpcodeTable[regOffset+104],eaStringOutput);
		}
	break;

	// group #3 Ev
	case 0xf7:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		if (regOffset<2) {
			sprintf(stringBuffer,groupOpcodeTable[regOffset+112],eaStringOutput,cpu->getmemuint16(cs,ip+instructionSize));
			instructionSize+=2;
		} else {
			sprintf(stringBuffer,groupOpcodeTable[regOffset+112],eaStringOutput);
		}
	break;

	// group #4 inc/dec
	case 0xfe:
		eaSize=8;
		explicitEaSize=true;
		decodemodrmbyte();
		if (regOffset==7) {
			sprintf(stringBuffer,groupOpcodeTable[regOffset+120],cpu->getmemuint8(cs,ip+2));
			instructionSize=3;
		} else {
			sprintf(stringBuffer,groupOpcodeTable[regOffset+120],eaStringOutput);
		}
	break;

	// group #5 inc/dec etc
	case 0xff:
		eaSize=16;
		explicitEaSize=true;
		decodemodrmbyte();
		sprintf(stringBuffer,groupOpcodeTable[regOffset+128],eaStringOutput);
	break;

	default:
		strcpy(stringBuffer,opcodeTable[opcode]);
		instructionSize=1;
	}
}
