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
#include"stdio.h"
#include"VideoDriverPalette13h.h"
#include"VideoDriverPalette1h.h"
#include"VideoDriverPalette3h.h"
#include"VideoDriverPaletteUndocumented.h"
#include"VideoDriverPalette4h.h"
#include"VideoDriverPalette5h.h"
#include"VideoDriverPalette6h.h"
#include"VideoDriverPaletteComposite.h"
#include"DiskImage.h"
#include"DebugOutput.h"
#include"MemoryMap.h"
#include"pic.h"
#include <assert.h>


extern CVideoDriver *theVideo;
extern CDiskImage *theFloppy;
extern CDebugOutput *theDebugOutput;
extern CPic *thePic;
extern uint8 awaitingKeypress;
extern uint8 *asciimap;
extern uint8 port61h;
extern int exitkey;
extern int state;

//#define printf //printf

uint32 Cpu86::nativeInterrupt(uint8 intNumber) {

	uint16 destSeg;
	uint16 destOfs;
	uint16 index;
	uint8 *src;
	uint8 inkey;
	uint16 tailPointer;
	uint16 headPointer;
	uint16 tailPointerPlus2;
	uint16 segment;
	uint16 offset;


	static int interruptHLE=0;
	interruptHLE=0;

	if (intNumber==0x13) {
		printf("interrupt %x ah=%x al=%x dl=%x\n",intNumber,ah,al,dl);
	}

	switch(intNumber) {
	case 0x8:
		setmemuint32(0,0x046c,getmemuint32(0,0x46c)+1);
		thePic->indicateNotBusy();
		break;
	case 0x1a:
		// get ticks since midnight
		dx=theCPU->memory[0x046c];
		cx=theCPU->memory[0x046e];
		al=theCPU->memory[0x470];
		theCPU->memory[0x470]=0;
		//printf("ticks since midnight: %X\n",theCPU->getmemuint32(0,0x046c));
		interruptHLE=1;
		break;
	case 0x19:
		// soft reboot, don't erase interrupt vector table
		cs=0;
		ip=0x7c00;
		interruptHLE=1;
		break;
	case 0x20:
		interruptHLE=1;
		break;
	case 0x16:
		//printf("int 16h, subfunction %d\n",ah);
		switch(ah) {
		case 0:
			// keyboard get keystroke

			headPointer=getmemuint16(0x40,0x1a);
			//if (headPointer!=getmemuint16(0x40,0x1c)) {

			// al contains ascii code, ah contains scan code

			al=getmemuint8(0x40,getmemuint16(0x40,0x1a)+1);
			ah=getmemuint8(0x40,getmemuint16(0x40,0x1a));

			headPointer+=2;	
			//printf("getting keystroke: %x\n",ah);

			if (headPointer>0x3c) {
				headPointer=0x1e;
			}
			setmemuint16(0x40,0x1a,headPointer);
			//} else {

				//printf("awaiting keypress\n");
				//awaitingKeypress=1;
				//while (getmemuint16(0x40,0x1a)==getmemuint16(0x40,0x1c)) {
					//printf("awaiting: %d\n",awaitingKeypress);
				//}

				//getchar();
				//printf("int16 ah=0 FIXME: wait for keyboard stroke\n");
				//printf("keyboard get keystroke, none available\n");
				// this should wait until a keypress is available.
				// wait for IRQ 1 to light up i guess
			//}

			interruptHLE=1;
			break;
		case 1:
			// keyboard check for keystroke
			// zeroFlag cleared if keystroke available

			zeroFlag=(getmemuint8(0x40,getmemuint16(0x40,0x1c))==getmemuint8(0x40,getmemuint16(0x40,0x1a)));
			setmemuint16(ss,sp+4,getmemuint16(ss,sp+4)&(65534<<6));
			setmemuint16(ss,sp+4,getmemuint16(ss,sp+4)|(zeroFlag<<6));

			if (zeroFlag) {
				ax=0;
			} else {
				//printf("int16 ah=1 keyboard check for keystroke: keystroke found\n");
				al=getmemuint8(0x40,getmemuint16(0x40,0x1a)+1);
				ah=getmemuint8(0x40,getmemuint16(0x40,0x1a));
			}

			//interruptHLE=1;

			//printf("keyboard check keystroke key %d, not available:%d, head %d, tail %d\n",ah,zeroFlag,getmemuint16(0x40,0x1a),getmemuint16(0x40,0x1c));

			break;
		case 2: // return shift flags in AL
			interruptHLE=1;
			al=0;
			break;
		}
		break;
	case 0x9: // keyboard hit hardware interrupt

		// head pointer: 0040:001a
		// tail pointer: 0040:001c (where next keystroke is placed, incremented by 2 when key is entered
		// buffer: 0040:001e to 0040:003d

		// checkkbd:
		// mov ax,40h
		// mov ds,ax
		// mov ax,[1ah]
		// cmp ax,[1ch]
		// je checkkbd

		catchPortIn8(0x60,&inkey);
		if ((inkey>>7)==0) {
			tailPointer=getmemuint16(0x40,0x1c);
			//printf("inserting scancode into buffer scan[%x], ascii[%x], tail %d, head %d\n",inkey,asciimap[inkey],tailPointer,getmemuint16(0x40,0x1a));
			tailPointerPlus2=tailPointer+2;
			if (tailPointerPlus2>0x3c) {
				tailPointerPlus2=0x1e;
			}

			if (tailPointerPlus2!=getmemuint16(0x40,0x1a)) {
				setmemuint8(0x40,getmemuint16(0x40,0x1c),inkey);
				setmemuint8(0x40,getmemuint16(0x40,0x1c)+1,asciimap[inkey]);
				tailPointer+=2;	
				if (tailPointer>0x3c) {
					tailPointer=0x1e;
				}
				setmemuint16(0x40,0x1c,tailPointer);
			}
		}
		port61h&=127;
		thePic->indicateNotBusy();
		break;
	case 0x17: // printer services:
		ah=192;
		interruptHLE=1;
		break;
	case 0x10: // video interrupt
		//printf("Video Interrupt ah=%x al=%x [%c,%c]\n",ah,al,ah,al);
		//sprintf(theDebugOutput->getBuffer(),"Interrupt 10, al=%x, ah=%x, bl=%x, bh=%x, cl=%x, ch=%x, dl=%x, dh=%x\n",*al,*ah,*bl,*bh,*cl,*ch,*dl,*dh);
		//theDebugOutput->writeMessage();
		switch(ah) {
		case 0: // set video mode
			printf("set video mode: %x",al);
			//getchar();
			switch(al&127) {
			case 0x0:
			case 0x1:
					if (((al)&128)==0) {
						memset(&memory[0xb8000],0,64000);
						printf("clearing video memory\n");
						for (index=0; index<64000; index+=2) {
							memory[0xb8001+index]=0x07;
						}
					}
					delete theVideo;
					theVideo=new CVideoDriverPalette1h(&theCPU->memory[0xb8000]);
					memory[MM_CURRENT_VIDEO_MODE_BYTE]=0x1;
					printf("Textmode 40x25\n");
					interruptHLE=1;
					// 40x25 text, 16 colours, 8 pages
				break;
			case 0x2:
			case 0x3:
					// 80x25 text, 16 colours, 8 pages
					if (((al)&128)==0) {
						printf("clearing video memory\n");
						memset(&memory[0xb8000],0,64000);
						for (index=0; index<64000; index+=2) {
							memory[0xb8001+index]=0x07;
						}

					}
					delete theVideo;
					theVideo=new CVideoDriverPalette3h(&theCPU->memory[0xb8000]);
					memory[MM_CURRENT_VIDEO_MODE_BYTE]=0x3;
					interruptHLE=1;
					printf("Textmode 80x25\n");
				break;
			case 0x4:
					printf("al = %d, clear = %d\n",al,((al)&128)==0);
					if (((al)&128)==0) {
						memset(&memory[0xb8000],0,64000);
						printf("clearing video memory\n");
					}
					delete theVideo;
					theVideo=new CVideoDriverPalette4h(&theCPU->memory[0xb8000]);
					memory[MM_CURRENT_VIDEO_MODE_BYTE]=0x4;
					interruptHLE=1;
					break;
			case 0x5:
					if (((al)&128)==0) {
						memset(&memory[0xb8000],0,64000);
						printf("clearing video memory\n");
					}
					delete theVideo;
					theVideo=new CVideoDriverPalette4h(&theCPU->memory[0xb8000]);
					memory[MM_CURRENT_VIDEO_MODE_BYTE]=0x4;
					interruptHLE=1;
					break;
			case 0x6:
					if (((al)&128)==0) {
						memset(&memory[0xb8000],0,64000);
						printf("clearing video memory\n");
					}
					delete theVideo;
					theVideo=new CVideoDriverPaletteComposite(&theCPU->memory[0xb8000]);
					//theVideo=new CVideoDriverPalette6h(&theCPU->memory[0xb8000]);
					memory[MM_CURRENT_VIDEO_MODE_BYTE]=0x6;
					interruptHLE=1;
					break;
			case 0x13:	// 320x200 Graphics, 256 colors, 1 page
					printf("Switching to VGA 320x200x256\n");
					if (((al)&128)==0) {
						memset(&memory[0xa0000],0,64000);
						printf("clearing video memory\n");
					}
					delete theVideo;
					theVideo=new CVideoDriverPalette13h(&theCPU->memory[0xA0000]);
					memory[MM_CURRENT_VIDEO_MODE_BYTE]=0x13;
					interruptHLE=1;
				break;
			case 0xd:
					// ega, 320x200, presumably 16 colours (a000)

				break;
			case 0xe:
					// ega, 640x200, not sure about colours (a000)
				break;
			case 0xf:
					// ega, 640x350, mono (a000)
				break;
			case 0x10:
					printf("Switching to EGA 640x350\n");
					interruptHLE=1;
					// ega, 640x350 a000 (16 colours, bitplaned)
					// this will need a memoryWatchZone to be set up i think				
					// http://webclass.cqu.edu.au/Units/81120_FOCT_Hardware/Study_Material/Study_Guide/chap2/chapter2.html
				break;
			case 0x12:
					printf("Switching to VGA 640x480x16\n");
					interruptHLE=1;
				break;
			}
			break;
		case 0x1: // define cursor appearance
			theVideo->setCursorSize(ch, cl);
			interruptHLE=1;
			break;
		case 0x2: // set cursor position:
			//printf("Set Cursor Position page %d column %d row %d\n",bh,dl,dh);
			theVideo->setCursorPosition(bh,dl,dh);
			interruptHLE=1;
			break;
		case 0x3:
			CURSORPOSITIONANDSIZE cursorPositionAndSize;
			theVideo->getCursorPosition(bh,&cursorPositionAndSize);
			dl=cursorPositionAndSize.column;
			dh=cursorPositionAndSize.row;
			cl=cursorPositionAndSize.topScanLine;
			ch=cursorPositionAndSize.bottomScanLine;
			interruptHLE=1;
			break;
		case 0x5: // set current display page:
			theVideo->setActivePage(al);
			interruptHLE=1;
		case 0x6: // scroll text lines up
			theVideo->scrollActivePageUp(ch,cl,dh,dl,bh,al);
			interruptHLE=1;
			break;
		case 0x7: // scroll text lines up
			theVideo->scrollActivePageDown(ch,cl,dh,dl,bh,al);
			interruptHLE=1;
			break;
		case 0x8:
			CHARACTERATTRIBUTE characterAttribute;
			theVideo->readCharacterAndAttribute(bh,&characterAttribute);
			al=characterAttribute.character;
			ah=characterAttribute.attribute;
			interruptHLE=1;
			break;
		case 0x9:
			theVideo->writeCharacterAndAttribute(al,bl,bh,cx);
			interruptHLE=1;
			break;
		case 0xa:
			theVideo->writeCharacterAtCursor(bh,al,cx);
			interruptHLE=1;
			break;
		case 0xb:
			// colour palette
			interruptHLE=1;
			break;
		case 0xc:
			// sets the colour number of the specified pixel in graphics mode
			// guessing this just writes directly to vidmem, need to check
			//printf("put pixel x=%d, y=%d, col=%d\n",cx,dx,al);
			theVideo->writeGraphicsPixel(cx,dx,al);
			interruptHLE=1;
			break;
		case 0xd:
			// reads the colour number of the specified pixel in graphics mode
			// guessing this just reads directly from vidmem, need to check
			interruptHLE=1;
			break;
		case 0xe:
			//printf("teletype write character (al='%c',bl=%d)\n",al,bl);
			theVideo->teletypeWriteCharacter(al,bl);
			interruptHLE=1;
			break;
		case 0x10:
			//printf("Unknown interrupt in DUNES.COM (palette?)\n");

			// following code disabled to allow MINESHAF.IMG to boot
			/*index=0;
			theVideo->catchPortOut8(0x3c8,bx*3);
			while (cx>0) {
				theVideo->catchPortOut8(0x3c9,theCPU->getmemuint8(es,dx+index));
				cx--;
				index++;
			}*/

			interruptHLE=1;

			// vga colour functions, palette and border
			// definitely need to be highlevel emulated
			// or could be passed down to port level for "fun"
			break;
		/*case 0x11:
			interruptHLE=1;
			break;
		case 0x12:
			ax=getmemuint16(0x0040,0x0013);
			interruptHLE=1;
			break;*/
		case 0xf:
			// return current video parameters
			// this will need to be done properly
			al=3;
			ah=80;
			bh=0;
			interruptHLE=1;
			break;
			
		}
		break;
		case 0x11: // BIOS machine information
			ax=theCPU->getmemuint16(0x40,0x10);
			printf("INT 11 Equipment List:\n");
			printf("* IPL diskette installed: %d",ax&1);
			printf("* Math coprocessor installed: %d",(ax>>1)&1);

			interruptHLE=1;
			break;
		case 0x12: // get memory size
			ax=getmemuint16(0x0040,0x0013);
			printf("get memory size: %d\n",ax);
			interruptHLE=1;
			break;
		case 0x21:
			printf("Called INT 21h, subfunction %x\n",ah);
			//exitkey=1;
			break;
		case 0x13:
			//printf("interrupt 0x13, function %d\n",ah);
			
			char *floppyBuffer;
			switch (ah) {
			case 0:
				ah=0;
				//carryFlag=0;

				if (dl==0) {
					//state=1;
					setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534) );

					segment=getmemuint16(0,(0x1e<<2)+2);
					offset=getmemuint16(0,0x1e<<2);

					/*printf("* INT 1E report taken from %x:%x:\n",segment,offset);
					printf("* Step rate and head unload time: %d\n",theCPU->getmemuint8(segment,offset));
					printf("* Head load time and DMA mode flag: %d\n",theCPU->getmemuint8(segment,offset+1));				
					printf("* Delay for motor turn off: %d\n",theCPU->getmemuint8(segment,offset+2));
					printf("* Bytes per sector: %d\n",theCPU->getmemuint8(segment,offset+3));
					printf("* Sectors per track: %d\n",theCPU->getmemuint8(segment,offset+4));
					printf("* Intersector gap length: %d\n",theCPU->getmemuint8(segment,offset+5));
					printf("* Data length: %d\n",theCPU->getmemuint8(segment,offset+6));
					printf("* Intersector gap length during format: %d\n",theCPU->getmemuint8(segment,offset+7));
					printf("* Format byte value: %x\n",theCPU->getmemuint8(segment,offset+8));
					printf("* Head settling time: %x\n",theCPU->getmemuint8(segment,offset+9));
					printf("* Delay until motor at normal speed: %x\n",theCPU->getmemuint8(segment,offset+10));
					printf("* Data area sector address: %d\n",(uint32)theCPU->getmemuint32(segment,offset+11));
					printf("* Cylinder number to read from: %d\n",theCPU->getmemuint16(segment,offset+15));
					printf("* Sector number to read from: %d\n",theCPU->getmemuint8(segment,offset+17));
					printf("* Root directory sector address: %d\n",theCPU->getmemuint32(segment,offset+19));*/

					/*switch(theCPU->getmemuint8(segment,offset+3)) {
						case 0:
							theFloppy->bytesPerSector=512;
							break;
						case 1:
							theFloppy->bytesPerSector=256;
							break;
						case 2:
							theFloppy->bytesPerSector=512;
							break;
						case 3:
							theFloppy->bytesPerSector=1024;
							break;
					}*/

					//theFloppy->sectors=theCPU->getmemuint8(segment,offset+4);
					carryFlag=0;
				} else {
					carryFlag=1;
				}
				setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534)|carryFlag );
				break;
			case 1:
				if (dl==0) {
					if (theFloppy!=NULL) {
						ah=theFloppy->getError();
						carryFlag=theFloppy->getError()!=0;
						interruptHLE=1;
					}
					carryFlag=0;
				} else {
					carryFlag=1;
				}
				setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534)|carryFlag );

				break;
			case 2:
				if (theFloppy!=NULL) {
					//printf("drive number %d\n",dl);
					if (dl==0) {
						floppyBuffer=new char[al*theFloppy->bytesPerSector];
						al=theFloppy->loadData(floppyBuffer,dh,ch,cl,al);
						ah=theFloppy->getError();
						//printf("error: %d\n",theFloppy->getError());
						setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534)|(ah!=0) );
						//printf("es=%x bx=%x\n",es,bx);
						src=(uint8 *)floppyBuffer;
						destSeg=es;
						destOfs=bx;
						index=al*theFloppy->bytesPerSector;
						while (index>0) {
							setmemuint8(destSeg,destOfs,*src);
							destOfs++;
							index--;
							src++;
						}
						delete []floppyBuffer;
						setmemuint8(0x40,0x40,0);
						interruptHLE=1;
						carryFlag=0;
					} else {
						carryFlag=1;
						setmemuint8(40,41,6);
					}
				} else {
					setmemuint8(40,41,6);
					carryFlag=1;
				}
				setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534)|carryFlag);
				break;
			case 3:
				ah=theFloppy->saveData((uint8 *)getptrmemuint8(es,bx),dh,ch,cl,al);
				interruptHLE=1;
				break;
			case 4: // verify sectors
				ah=0;
				carryFlag=0;
				interruptHLE=1;
				break;
			case 8:
				if (dl==0) {
					if (theFloppy!=NULL) {
						ah=0;
						bl=2;
						dh=theFloppy->heads-1;  	// head starts at 0
						ch=theFloppy->tracks-1;		// track aka cylinder starts at 0
						cl=theFloppy->sectors;	 	// sector starts at 1
						dl=0;
						es=getmemuint16(0,(0x1e<<2)+2);
						di=getmemuint16(0,(0x1e<<2));

						//setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534) );

						segment=getmemuint16(0,(0x1e<<2)+2);
						offset=getmemuint16(0,0x1e<<2);

						printf("* INT 1E report taken from %x:%x:\n",segment,offset);
						printf("* Step rate and head unload time: %d\n",theCPU->getmemuint8(segment,offset));
						printf("* Head load time and DMA mode flag: %d\n",theCPU->getmemuint8(segment,offset+1));				
						printf("* Delay for motor turn off: %d\n",theCPU->getmemuint8(segment,offset+2));
						printf("* Bytes per sector: %d\n",theCPU->getmemuint8(segment,offset+3));
						printf("* Sectors per track: %d\n",theCPU->getmemuint8(segment,offset+4));
						printf("* Intersector gap length: %d\n",theCPU->getmemuint8(segment,offset+5));
						printf("* Data length: %d\n",theCPU->getmemuint8(segment,offset+6));
						printf("* Intersector gap length during format: %d\n",theCPU->getmemuint8(segment,offset+7));
						printf("* Format byte value: %x\n",theCPU->getmemuint8(segment,offset+8));
						printf("* Head settling time: %x\n",theCPU->getmemuint8(segment,offset+9));
						printf("* Delay until motor at normal speed: %x\n",theCPU->getmemuint8(segment,offset+10));
						printf("* Data area sector address: %d\n",(uint32)theCPU->getmemuint32(segment,offset+11));
						printf("* Cylinder number to read from: %d\n",theCPU->getmemuint16(segment,offset+15));
						printf("* Sector number to read from: %d\n",theCPU->getmemuint8(segment,offset+17));
						printf("* Root directory sector address: %d\n",theCPU->getmemuint32(segment,offset+19));
						interruptHLE=1;
						state=1;

					} 
				} else {
					// dos checks for return values of 0x11, 0x9 and 0x6 on exit from here
					carryFlag=1;
				}
				setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534)|carryFlag );

				break;
			case 15:
				if (dl==0) {
					ah=1;
					carryFlag=0;
				} else {
					ah=0;
					carryFlag=0;
				}
				setmemuint16(ss,sp+4, (getmemuint16(ss,sp+4)&65534)|carryFlag );
				break;
			default:
				ah=1;
			}
			break;


// end of fdd
	
	}


	return(interruptHLE);
}

