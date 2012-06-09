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

#include"cpu86.h"
#include"VideoDriver.h"
#include"pic.h"
#include"pit.h"
#include"VideoDriverPalette1h.h"
#include"VideoDriverPalette3h.h"
#include"VideoDriverPaletteUndocumented.h"
#include"VideoDriverPalette4h.h"
#include"VideoDriverPalette5h.h"
#include"VideoDriverPalette6h.h"
#include"VideoDriverPaletteComposite.h"
#include"assert.h"

extern CVideoDriver *theVideo;
extern CPic *thePic;
extern CPit *thePit;
extern uint8 lastScancode;
extern uint8 lastIn61h;
extern uint8 *asciimap;
extern uint8 port61h;
extern uint8 port40h;
extern uint8 last3D4h;

//#define printf //printf

uint32 Cpu86::catchPortOut8(uint16 portNumber,uint8 value) {
	static int outportHLE;


	if (portNumber>0x201)
		printf("%x:%x Running catchPortOut8, port = %X value = %X\n",cs,ip,portNumber,value);

	outportHLE=0;
	switch(portNumber) {
	case 0x20:
		//printf("Interrupt Flag: %d, isBusy: %d\n",interruptFlag,thePic->isBusy());
		if (value==0x20) {
			thePic->indicateNotBusy();
		}
		// hand control back to PIC
		break;
	case 0x21:
		thePic->setLines(value);
		//printf("Setting interrupt lines\n");
		//getchar();
		break;
	case 0x3c8:
		// set palette entry to be modified to value
		printf("sending %x to port 3c8\n",value);
		theVideo->catchPortOut8(portNumber,value);
		outportHLE=1;
		break;
	case 0x3c9:
		printf("sending %x to port 3c9\n",value);
		theVideo->catchPortOut8(portNumber,value);
		// set r,g or b value of above entry, and move to next (ie r to g, g to b, b to r?)
		outportHLE=1;
		break;
	case 0x3d4:
		last3D4h=value;
		break;
	case 0x3d5:
		if (last3D4h<=0xf) {
			lastPortValueWritten[last3D4h]=value;
			if (	(lastPortValueWritten[0x3]==0xA) &&
				(lastPortValueWritten[0x4]==0x7C) &&
				(lastPortValueWritten[0x5]==0x6) &&
				(lastPortValueWritten[0x6]==0x64) &&
				(lastPortValueWritten[0x7]==0x70) &&
				(lastPortValueWritten[0x8]==0x2) &&
				(lastPortValueWritten[0x9]==0x1) &&
				(lastPortValueWritten[0xA]==0x20) &&
				(lastPortValueWritten[0xB]==0x0) &&
				(lastPortValueWritten[0xC]==0x0) &&
				(lastPortValueWritten[0xD]==0x0) &&
				(lastPortValueWritten[0xE]==0x38) &&
				(lastPortValueWritten[0xF]==0x28)) {
					delete theVideo;
					theVideo=new CVideoDriverPaletteUndocumented(&theCPU->memory[0xb8000]);

			}

		}
		break;
	case 0x3d8:
		switch (value&63) {
			case 44:
			case 40:
				if (getmemuint8(0x40,0x49)!=1) {
					delete theVideo;
					theVideo=new CVideoDriverPalette1h(&theCPU->memory[0xb8000]);
					setmemuint8(0x40,0x49,1);
				}
				break;
			case 45:
			case 41:
				if (getmemuint8(0x40,0x49)!=3) {
					delete theVideo;
					theVideo=new CVideoDriverPalette3h(&theCPU->memory[0xb8000]);
					setmemuint8(0x40,0x49,3);
				}
				break;
			case 14:
			case 10:
				if (getmemuint8(0x40,0x49)!=4) {
					//delete theVideo;
					theVideo=new CVideoDriverPalette4h(&theCPU->memory[0xb8000]);
					setmemuint8(0x40,0x49,4);
				}
				break;
			case 46:
				delete theVideo;
				theVideo=new CVideoDriverPalette5h(&theCPU->memory[0xb8000]);
				break;
			case 42:
			case 26:
				delete theVideo;
				theVideo=new CVideoDriverPaletteComposite(&theCPU->memory[0xb8000]);
				break;
		}
	case 0x3d9:
		// used by styx.img
		theVideo->catchPortOut8(portNumber,value);
		break;
	case 0x3f4:
		// used by congo.img
		break;
	case 0x40:
	case 0x41:
	case 0x42:
		//printf("Write counter %d value: %d\n",portNumber-0x40,value);
		thePit->writeCounterValue(portNumber-0x40,value);
		//printf("Counter %d value: %d\n",portNumber-0x40,thePit->getCounterValue(portNumber-0x40));
		break;
	case 0x43:
		thePit->writeControlByte(value);
		/*printf("* PIT Control Word Write:\n");
		switch((value>>1)&7) {
			case 0:
				printf("Mode 0: Interrupt on Terminal Count\n");
				break;
			case 1:
				printf("Mode 1: Programmable One-Shot\n");
				break;
			case 2:
				printf("Mode 2: Rate Generator\n");
				break;
			case 3:
				printf("Mode 3: Square Wave Rate Generator\n");
				break;
			case 4:
				printf("Mode 4: Software Triggered Strobe\n");
				break;
			case 5:
				printf("Mode 5: Hardware Triggered Strobe\n");
				break;
		}
		switch((value>>4)&3) {
			case 0:
				printf("Read/Load: Counter Latching\n");
				break;
			case 1:
				printf("Read/Load: LSB Only\n");
				break;
			case 2:
				printf("Read/Load: MSB Only\n");
				break;
			case 3:
				printf("Read/Load: MSB then LSB\n");
				break;
		}
		printf("Counter Number %d\n",value>>6);*/

		// timer chip has a 1.19318mhz clock speed
		// to set out timer to a given hz we must take this number and divide it by the hz
		// 8253 timer: timer control 
		// 1,193,180 / 65535 gives us 18.2hz

		
/* 

http://courses.ece.uiuc.edu/ece291/books/labmanual/io-devices-timer.html
really good url
  
	Bits 7,6:
Channel ID (11 is illegal)
 
Bits 5,4:
 Read/load mode for two-byte count value:

00 -- latch count for reading 
01 -- read/load high byte only 
10 -- read/load low byte only 
11 -- read/load low byte then high byte 
 
Bits 3,2,1:
 Count mode selection (000 to 101)
 
Bit 0:
 0/1: Count in binary/BCD */
 

		
		break;
	case 0x61:
		// port 61h
		// bits 0-1 pc speaker
		// bit 6 keyboard clock enable/disable
		// bit 7 keyboard acknowledge

		//printf("port 61h out: %x (%d)\n",value,value);
		//assert(0);
		//port61h=value;
		break;

	}
	return(outportHLE);
}

// pull a value from a port into an 8bit register

uint8 Cpu86::catchPortIn8(uint16 portNumber, uint8 *destination) {
	static int inportHLE;
	//printf("Running catchPortIn8, %X:%X port = %X\n",cs,ip,portNumber);
	switch(portNumber) {
	case 0x20:
		break;
	case 0x21:
		*destination=thePic->readLines();

		break;

	case 0xa0:
		// used by galaxian
		// NMI mask register - see ports.a
		break;
	case 0x3da:
		portReadsTillNextRetraceChange--;
		if (portReadsTillNextRetraceChange==0) {
			switch(verticalRetrace) {
			case 0:
				verticalRetrace=1;
				break;
			case 1:
				verticalRetrace=8;
				break;
			case 8:
				verticalRetrace=9;
				break;
			case 9:
				verticalRetrace=0;
				break;
			}
			portReadsTillNextRetraceChange=3;
		}
		*destination=verticalRetrace;

		inportHLE=1;
		break;
	case 0x3d9:
		*destination=0xff;
		inportHLE=1;
		break;
	case 0x3f4:
		*destination=128;
		inportHLE=1;
		break;
	case 0x60: // keyboard
		*destination=lastScancode;
		//printf("port 60h in: %x (%d)\n",lastScancode,lastScancode);
		//keypressed+=127;
		break;
	case 0x61: // something to do with keyboard
		/*if (lastIn61h&1) {
			lastIn61h&=254;
		} else {
			lastIn61h|=1;
		}*/
		//printf("port 61h in: %x (%d)\n",port61h,port61h);
		*destination=port61h;
		break;
	case 0x201:
		// joystick
		*destination=0xff;
		break;
	case 0x40:
		// port 40 used by archon
		//printf("port 40h: %d\n",port40h);
		printf("Read counter 0 value\n");
		*destination=port40h;
		break;
	case 0x41:
		// 8253 timer: channel 1 count register (memory refresh?)
		printf("Read counter 1 value\n");
		break;
	case 0x42:
		// 8253 timer: channel 2 count register (tape drive/speaker?)
		printf("Read counter 2 value\n");
		break;

	}
	return(inportHLE);
}
