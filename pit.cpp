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

#include "pit.h"


CPit::CPit() {
	counterValue=new uint16[3];
	portState=3;
}

void CPit::writeControlByte(uint8 value) {

	// portState:
	//  lsb only: 0 (change to 4 when done)
	//  msb only: 1 (change to 4 when done)
	//  msb then lsb (2 then 3 then change to 4 when done)

	printf("Control Byte:\n");
	switch((value>>4)&3) {
		case 0:
			// Read/Load Counter Latching
			counterLatchValue=counterValue[value>>6];
			portState=4;
			break;
		case 1:
			printf("setting portstate to 0 (lsb only)\n");
			portState=0; // read/load lsb only
			break;
		case 2:
			printf("setting portstate to 1 (msb only)\n");
			portState=1; // read/load msb only
			break;
		case 3:
			printf("setting portstate to 2 (msb then lsb)\n");
			portState=2; // read/load msb then lsb
			break;
	}
}

void CPit::writeCounterValue(uint8 counter,uint8 value) {
	switch(portState) {
		case 0:
			printf("setting LSB of counter %d to %d\n",counter,value);
			counterValue[counter]=(counterValue[counter]&0xff00)|value;
			portState=3;
			printf("Final value of counter %d: %d\n",counter,counterValue[counter]);
			break;
		case 1:
			printf("setting MSB of counter %d to %d\n",counter,value);
			counterValue[counter]=(counterValue[counter]&0x00ff)|((uint16)value)<<8;
			portState=3;
			printf("Final value of counter %d: %d\n",counter,counterValue[counter]);
			break;
		case 2:
			printf("setting MSB (prior to LSB) of counter %d to %d\n",counter,value);
			counterValue[counter]=(counterValue[counter]&0x00ff)|((uint16)value)<<8;
			portState=0;
			break;
		case 3:
			// think what happens here depends on current mode
			break;
	}
}

uint16 CPit::getCounterValue(uint8 counter) {
	return(counterValue[counter]);
}

CPit::~CPit() {
	delete [] counterValue;
}
