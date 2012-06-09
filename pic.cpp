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
#include <stdarg.h>

#include <assert.h>

#include "pic.h"

extern uint8 lastScancode;

CPic::CPic() {

	//int irqServicing; // set to -1 if not busy

	irqServicing=-1;
	irqLookups=new uint32[8];
	irqLines=new uint32[8];
	irqsEnabled=new uint32[8];

	irqLookups[0]=0x08;
	irqLookups[1]=0x09;
	irqLookups[2]=0x0A;
	irqLookups[3]=0x0B;
	irqLookups[4]=0x0C;
	irqLookups[5]=0x0D;
	irqLookups[6]=0x0E;
	irqLookups[7]=0x0F;

	for (uint32 index=0; index<8; index++) {
		irqLines[index]=0;
		irqsEnabled[index]=0; // 0 = enabled, 1 = disabled
	}
}

CPic::~CPic() {
	delete []irqsEnabled;
	delete []irqLines;
	delete []irqLookups;
}

sint32 CPic::getNextInterrupt() {
	sint32 returnValue=-1;
	int index=0;
	int sorted=0;
	while ((index<8) && (!sorted)) {
		//printf("irq %d enabled %d\n",irqLines[index],irqsEnabled[index]);
		if ((irqLines[index]==1) && (irqsEnabled[index]==0)) {
            //printf("setting return value to %d\n",irqLookups[index]);
			returnValue=irqLookups[index];
			irqLines[index]=0;
			irqServicing=index;
			sorted=1;
			if (index==1) {
				printf("pic.cpp scanning active interrupts, found: (code: %x(x) %d(d)) %x\n",lastScancode,lastScancode,index);
			}
		}
		index++;
	}
	return(returnValue);
}

void CPic::indicateNotBusy() {
	if (irqServicing>0) {
		//printf("PIC notified of ISR completion for IRQ %d\n",irqServicing);
	}
	irqServicing=-1;
}

void CPic::irqRequestAttention(uint32 irqLine) {
	if (irqLine>0) {
		printf("Lighting up IRQ line %d\n",irqLine);
	}
	irqLines[irqLine]=1;
}

void CPic::setLines(uint8 lines) {
	for (int index=0; index<8; index++) {
		irqsEnabled[index]=((lines>>index)&1);
		/*if (irqsEnabled[index]) {
			printf("enabling IRQ %d\n",index);
		} else {
			printf("disabling IRQ %d\n",index);
		}*/
	}
}

uint8 CPic::readLines() {
	uint8 output=0;
	for (int index=0; index<8; index++) {
		/*if (irqsEnabled[index]) {
			printf("(read) IRQ %d enabled\n",index);
		} else {
			printf("(read) IRQ %d disabled\n",index);
		}*/
		//printf("irqsEnabled[%d]=%d\n",index,irqsEnabled[index]);
		output|=irqsEnabled[index];
		//printf("cumulative output: %d\n",output);
		if (index<7) {
			output<<=1;
		}
	}
	//printf("Generated output: %d\n",output);
	return(output);
}

sint32 CPic::isBusy() {
	return(irqServicing);
}

