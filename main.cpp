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


#include "cpu86.h"
#include "DiskImage.h"
#include "VideoDriverPalette13h.h"
#include "VideoDriverPalette1h.h"
#include "VideoDriverPalette3h.h"
#include "VideoDriverPalette4h.h"
#include "configuration.h"
#include "DebugOutput.h"
#include "pic.h"
#include "pit.h"
#include "disasm.h"
#include <SDL.h>

CPic *thePic;
CPit *thePit;
Cpu86 *theCPU;
CDisasm86 *theDisasm;
CDiskImage *theFloppy;
CVideoDriver *theVideo;
CDebugOutput *theDebugOutput;
SDL_Surface *screen;
SDL_TimerID pitTimer;
Uint8 *ubuff8; 
uint8 lastScancode;
uint8 testScancode;
uint8 lastAsciicode;
uint8 lastIn61h;
uint8 port61h;
uint8 port40h;
uint8 last3D4h;
uint8 awaitingKeypress=0;
int state=1;
int dropOut;
int validEvent;
int recording=0;
int showCursor=0;

uint8* keymap;
uint8* asciimap;
int exitkey = 0;
int cycles=0;
sint32 nextInterrupt;
SDL_Event event;
int instructions;
int howManyInstructions;
int overFlow;
int interruptCounter=5;
int busy=0;
int outerIndex;
int eventReady;

long int fsize (char *filename)
{
	FILE *pFile;
	long int lPos;
	pFile = fopen(filename, "rb");
	if (NULL == pFile)
		return (-1);
	fseek(pFile, 0, SEEK_END);
	lPos = ftell(pFile);
	fclose(pFile);
	return (lPos);
}


int main(int argc, char**argv) {

// KEYMAP START

keymap=new uint8[256];
asciimap=new uint8[256];


#define keymapold 1

#ifdef keymapold
keymap[0x16]=0x0e;	// backspace
asciimap[0x0e]=0x08;
keymap[0x42]=0x3a;	// caps lock

keymap[0x24]=0x1c;	// enter
asciimap[0x1c]=0x0d;
keymap[0x9]=0x01;	// esc
asciimap[0x1]=0x1b;
keymap[0x40]=0x38;	// left alt

keymap[0x25]=0x1d;	// left ctrl

keymap[0x32]=0x2a;	// left shift

keymap[0x4d]=0x45;	// num lock

keymap[0x3e]=0x36;	// right shift

keymap[0x4e]=0x46;	// scroll lock

keymap[0x41]=0x39;	// space
asciimap[0x39]=0x20;
keymap[0x6f]=0x54;	// sys req

keymap[0x17]=0x0f;	// tab
asciimap[0x0f]=0x9;
keymap[0x62]=0x48;	// up arrow
keymap[0x68]=0x50;	// down arrow
keymap[0x64]=0x4b;	// left arrow
keymap[0x66]=0x4d;	// right arrow

keymap[0x43]=0x3b;	// F1
keymap[0x44]=0x3c;	// F2
keymap[0x45]=0x3d;	// F3
keymap[0x46]=0x3e;	// F4
keymap[0x49]=0x41;	// F7
keymap[0x47]=0x3f;	// F5
keymap[0x48]=0x40;	// F6
keymap[0x4a]=0x42;	// F8
keymap[0x4b]=0x43;	// F9
keymap[0x4c]=0x44;	// F10
keymap[0x5f]=0x57;	// F11
keymap[0x60]=0x58;	// F12


keymap[0x26]=0x1e;	// A
asciimap[0x1e]=0x61;
keymap[0x38]=0x30;	// B
asciimap[0x30]=0x62;
keymap[0x36]=0x2e;	// C
asciimap[0x2e]=0x63;
keymap[0x28]=0x20;	// D
asciimap[0x20]=0x64;
keymap[0x1a]=0x12;	// E
asciimap[0x12]=0x65;
keymap[0x29]=0x21;	// F
asciimap[0x21]=0x66;
keymap[0x2a]=0x22;	// G
asciimap[0x22]=0x67;
keymap[0x2b]=0x23;	// H
asciimap[0x23]=0x68;
keymap[0x1f]=0x17;	// I
asciimap[0x17]=0x69;
keymap[0x2c]=0x24;	// J
asciimap[0x24]=0x6a;
keymap[0x2d]=0x25;	// K
asciimap[0x25]=0x6b;
keymap[0x2e]=0x26;	// L
asciimap[0x26]=0x6c;
keymap[0x3a]=0x32;	// M
asciimap[0x32]=0x6d;
keymap[0x39]=0x31;	// N
asciimap[0x31]=0x6e;
keymap[0x20]=0x18;	// O
asciimap[0x18]=0x6f;
keymap[0x21]=0x19;	// P
asciimap[0x19]=0x70;
keymap[0x18]=0x10;	// Q
asciimap[0x10]=0x71;
keymap[0x1b]=0x13;	// R
asciimap[0x13]=0x72;
keymap[0x27]=0x1f;	// S
asciimap[0x1f]=0x73;
keymap[0x1c]=0x14;	// T
asciimap[0x14]=0x74;
keymap[0x1e]=0x16;	// U
asciimap[0x16]=0x75;
keymap[0x37]=0x2f;	// V
asciimap[0x2f]=0x76;
keymap[0x19]=0x11;	// W
asciimap[0x11]=0x77;
keymap[0x35]=0x2d;	// X
asciimap[0x2d]=0x78;
keymap[0x1d]=0x15;	// Y
asciimap[0x15]=0x79;
keymap[0x34]=0x2c;	// Z
asciimap[0x2c]=0x7a;

keymap[0xa]=0x2;	// 1
asciimap[0x2]=0x31;
keymap[0xb]=0x3;	// 2
asciimap[0x3]=0x32;
keymap[0xc]=0x4;	// 3
asciimap[0x4]=0x33;
keymap[0xd]=0x5;	// 4
asciimap[0x5]=0x34;
keymap[0xe]=0x6;	// 5
asciimap[0x6]=0x35;
keymap[0xf]=0x7;	// 6
asciimap[0x7]=0x36;
keymap[0x10]=0x8;	// 7
asciimap[0x8]=0x37;
keymap[0x11]=0x9;	// 8
asciimap[0x9]=0x38;
keymap[0x12]=0xa;	// 9
asciimap[0xa]=0x39;
keymap[0x13]=0xb;	// 0
asciimap[0xb]=0x30;

keymap[0x14]=0x0c;	// -
keymap[0x15]=0x0d;	// =
keymap[0x22]=0x1a;	// [
keymap[0x23]=0x1b;	// ]
keymap[0x47]=0x27;	// ;
keymap[0x48]=0x28;	// '
keymap[0x31]=0x29;	// `
keymap[0x5e]=0x2b;	// backslash
keymap[0x3b]=0x33;	// ,
keymap[0x3c]=0x34;	// .
keymap[0x3d]=0x35;	// /
#endif


#ifdef keymapnew

keymap[0x16]=0x0e;	// backspace
keymap[0x42]=0x3a;	// caps lock
keymap[0x24]=0x1c;	// enter
keymap[0x9]=0x01;	// esc
keymap[0x40]=0x38;	// left alt
keymap[0x25]=0x1d;	// left ctrl
keymap[0x32]=0x2a;	// left shift
keymap[0x4d]=0x45;	// num lock
keymap[0x3e]=0x36;	// right shift
keymap[0x4e]=0x46;	// scroll lock
keymap[0x41]=0x39;	// space
keymap[0x6f]=0x54;	// sys req
keymap[0x17]=0x0f;	// tab

keymap[0x62]=0x48;	// up arrow
keymap[0x68]=0x50;	// down arrow
keymap[0x64]=0x4b;	// left arrow
keymap[0x66]=0x4d;	// right arrow

keymap[0x43]=0x05;	// F1
keymap[0x44]=0x06;	// F2
keymap[0x45]=0x04;	// F3
keymap[0x46]=0x0c;	// F4
keymap[0x49]=0x83;	// F7
keymap[0x47]=0x03;	// F5
keymap[0x48]=0x0b;	// F6
keymap[0x4a]=0x0a;	// F8
keymap[0x4b]=0x01;	// F9
keymap[0x4c]=0x09;	// F10
keymap[0x5f]=0x78;	// F11
keymap[0x60]=0x07;	// F12

keymap[0x26]=0x1c;	// A
keymap[0x38]=0x32;	// B
keymap[0x36]=0x21;	// C
keymap[0x28]=0x23;	// D
keymap[0x1a]=0x24;	// E
keymap[0x29]=0x2b;	// F
keymap[0x2a]=0x34;	// G
keymap[0x2b]=0x33;	// H
keymap[0x1f]=0x43;	// I
keymap[0x2c]=0x3b;	// J
keymap[0x2d]=0x42;	// K
keymap[0x2e]=0x4b;	// L
keymap[0x3a]=0x3a;	// M

keymap[0x39]=0x31;	// N
keymap[0x20]=0x44;	// O
keymap[0x21]=0x4d;	// P
keymap[0x18]=0x15;	// Q
keymap[0x1b]=0x2d;	// R
keymap[0x27]=0x1b;	// S
keymap[0x1c]=0x2c;	// T
keymap[0x1e]=0x3c;	// U
keymap[0x37]=0x2a;	// V
keymap[0x19]=0x1d;	// W
keymap[0x35]=0x22;	// X
keymap[0x1d]=0x35;	// Y
keymap[0x34]=0x1a;	// Z

keymap[0xa]=0x16;	// 1
keymap[0xb]=0x1e;	// 2
keymap[0xc]=0x26;	// 3
keymap[0xd]=0x25;	// 4
keymap[0xe]=0x2e;	// 5
keymap[0xf]=0x36;	// 6
keymap[0x10]=0x3d;	// 7
keymap[0x11]=0x3e;	// 8
keymap[0x12]=0x46;	// 9
keymap[0x13]=0x45;	// 0

keymap[0x14]=0x4e;	// -
keymap[0x15]=0x55;	// =
keymap[0x22]=0x54;	// [
keymap[0x23]=0x5b;	// ]
keymap[0x47]=0x4c;	// ;
keymap[0x48]=0x52;	// '
keymap[0x31]=0x0e;	// `
keymap[0x5e]=0x5d;	// backslash
keymap[0x3b]=0x41;	// ,
keymap[0x3c]=0x49;	// .
keymap[0x3d]=0x4a;	// /

#endif

// KEYMAP END


	howManyInstructions=1850;
	thePic=new CPic();
	thePit=new CPit();
	theDebugOutput=new CDebugOutput();
	theCPU=new Cpu86();
	theDisasm=new CDisasm86(theCPU);
	theVideo=new CVideoDriverPalette3h(&theCPU->memory[0xb8000]);

	int fileSize=fsize(argv[1]);

	if (fileSize==-1) {
		printf("Image file not found\n");
		return(1);
	} else {
		printf("Image file: %s\n",argv[1]);
	}

	theFloppy=NULL;
	printf("Floppy image: %dk\n",fileSize/1024);
	switch(fileSize) {
		case 320*1024:
			theFloppy=new CDiskImage(2,40,8); // 320k
			printf("320k disk (2,40,8)\n");
		break;
		case 180*1024:
			theFloppy=new CDiskImage(1,40,9); // 180k
			printf("180k disk (1,40,9)\n");
		break;
		case 200*1024:
			theFloppy=new CDiskImage(1,40,10); // 200k
			printf("200k disk (1,40,10)\n");
		break;
		case 160*1024:
			theFloppy=new CDiskImage(1,40,8); // 160k
			printf("160k disk (1,40,8)\n");
		break;
		case 360*1024:
			theFloppy=new CDiskImage(2,40,9); // 360k
			printf("360k disk (2,40,9)\n");
		break;
		case 720*1024:
			theFloppy=new CDiskImage(2,40,18); // 720k
			printf("720k disk (2,40,18)\n");
		break;
		case 1440*1024:
			theFloppy=new CDiskImage(2,80,18); // 1440k
			printf("1440k disk (2,80,18)\n");
		break;
		case 400*1024:
			theFloppy=new CDiskImage(2,40,10); // 400k
			printf("400k disk (2,40,10)\n");
		break;
		default:
			if (strchr(argv[1],'.')!=NULL) {
				char *ext=argv[1]+strlen(argv[1]);
				while (*ext!='.') {
					ext--;
				}
				printf("ext: [%s]\n",ext);
				if ((strcmp(ext,".COM")==0) || (strcmp(ext,".com")==0)) {
					printf("Image is .COM format");
					theCPU->cs=theCPU->ds=theCPU->ss=0x151f;
					theCPU->ip=0x100;
					FILE *image;
					image=fopen(argv[1],"rb+");
					if (image!=NULL) {
						fseek(image,0,SEEK_SET);
						fread(&theCPU->memory[0x152f0],fileSize,1,image);
						fclose(image);
					} else {
						printf("Unable to open %s\n",argv[1]);
					}
					
				} else {
					printf("Unknown disk image format (1)\n");
					return(1);
				}
			} else {
				printf("Unknown disk image format (2)\n");
				return(1);
			}
	} 
	if (theFloppy!=NULL) {
		int dog=theFloppy->insertMedia(argv[1]);
		if (dog!=0) {
			printf("Error inserting media\n");
			return(1);
		} else {
			printf("Media inserted OK\n");
			theFloppy->loadData((char *)(theCPU->memory+0x7c00),0,0,1,1);
		}
	}

	theCPU->reset();

	CConfiguration *theConfiguration=new CConfiguration();
	theConfiguration->set8087(CONFIGURATION_NOT_INSTALLED);
	theConfiguration->setMouse(CONFIGURATION_NOT_INSTALLED);
	theConfiguration->setInitialVideoMode(CONFIGURATION_INITIAL_VIDEO_MODE_80_25_CGA);
	theConfiguration->setNumDisketteDrives(1);
	theConfiguration->setNumSerialAdapters(0);
	theConfiguration->setGameAdapter(CONFIGURATION_NOT_INSTALLED);
	theConfiguration->setNumParallelAdapters(0);
	theCPU->setmemuint16(0x40,0x10,theConfiguration->getConfigurationuint16());
	delete theConfiguration;

	theVideo->setCursorPosition(0,0,0);

	/* Initialize SDL, exit if there is an error. */
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Could not initialize SDL: %s\n", 
		SDL_GetError());
		return(-1);
	}
	SDL_WM_SetCaption("Legacy (trunk)","Legacy");
  
	/* Grab a surface on the screen */
	screen = SDL_SetVideoMode(640, 400, 32, SDL_SWSURFACE|SDL_ANYFORMAT);
	if( !screen ) {
		fprintf(stderr, "Couldn't create a surface: %s\n",
		SDL_GetError());
		return -1;
	}
	ubuff8 = (Uint8*) screen->pixels;

	while (!exitkey) {

		if (recording) {
			theDisasm->matchCSIP();
			theDisasm->decodeInstruction();
			printf("%x:%x %s (%x:%x)\n",theCPU->cs,theCPU->ip,theDisasm->stringBuffer,theCPU->getmemuint16(0,(0x1e<<2)+2),theCPU->getmemuint16(0,(0x1e<<2)));
		}

		/*if ((theCPU->cs==0x50) && (theCPU->ip==0x20)) {
			state=1;
		}*/

		theCPU->execute();
		if (theCPU->interruptFlag) {
			nextInterrupt=thePic->getNextInterrupt();	
			if (nextInterrupt!=-1) {
                //printf("next interrupt is %d\n",nextInterrupt);
				//theCPU->setmemuint8(0x40,0x40,0); // time out floppy disk motor
				if (nextInterrupt==8) {
					//printf("executing int8: cs=%x, ip=%x\n",theCPU->cs,theCPU->ip);
				}
                //printf("running hardware interrupt\n");
				theCPU->hardwareInterrupt(nextInterrupt);
			}
		}

		cycles++;
		if (cycles>25000) {

			//if ((thePic->readLines()&1)==0) {
				thePic->irqRequestAttention(0);
				port40h--;
			/*} else {
				printf("clock tick not occurred (interrupt disabled %d)\n",thePic->readLines());
			}*/

			if (state==0) {
				theVideo->render();
			}
			//SDL_Delay(10);
			cycles=0;

			showCursor++;
			if (showCursor==16) {
				showCursor=0;
			}
			theVideo->setCursorDisplay(showCursor>>3);

		}
		/*if (state==1) {
		}*/

		eventReady=0;
		if (state==1) {
			dropOut=false;
			while (!dropOut) {
				theDisasm->render();
				eventReady=SDL_WaitEvent(&event);
				if (event.type==SDL_KEYDOWN) {
					testScancode=event.key.keysym.scancode;
					switch(testScancode) {
						case 67: // F1
							//dropOut=true;
							dropOut=true;
							printf("cs: %x,ip: %x\n",theCPU->cs,theCPU->ip);
						break;
						case 76: // F10
							//solo
							//theCPU->ip=0x591;
							//f15cga
							//theCPU->ip=0x2dcf;
							//hellcat
							theCPU->ip=0x872;

						break;

						case 95: // F11
							//theFloppy->ejectMedia();
							//theFloppy->insertMedia("/home/jonathan/legacy010405/images/KQ2-2.IMG");
							//printf("Inserting Wiz Disc\n");
							//dropOut=true;
							/*printf("Recording=on\n");
							recording=1;*/
							// pitstop 2 patch - apply at beginning of race
							theCPU->setmemuint16(0x0050,0x0f83,0x9090);
							//spitfire
							//theCPU->ip=0x423;
							//f15cga
							//theCPU->ip=0x2d57;
							//solo
							//theCPU->ip=0x3557;
							//hellcat
							//theCPU->ip=0x421;
							//alleycat
							//theCPU->ip=0x62d7;							
						break;
						case 96: // F12
							state=0;
							dropOut=true;
						break;
					}
				} else {
					if (event.type==SDL_QUIT) {
						exitkey = 1;
						state=0;
						dropOut=true;
				  		//printf("Got quit event!\n");
				  	}
				}
			}
		} 

		eventReady=SDL_PollEvent(&event);
		if(eventReady){ 
			switch(event.type) {
				case SDL_QUIT:
					exitkey = 1;
			  	break;
				case SDL_KEYDOWN:
					testScancode=event.key.keysym.scancode;
					if (state==0) {
						if (testScancode==96) {
							// go into debugger
							state=1;
						} else {
							lastScancode=keymap[testScancode];	
							printf("*** SDL caught keypress: %d\n",event.key.keysym.scancode);
							lastAsciicode=asciimap[lastScancode];
							thePic->irqRequestAttention(1);
							awaitingKeypress=0;
							//state=1;

						}
					} else {
					}
				break;
				case SDL_KEYUP:
					if (state==0) {
						lastScancode=keymap[event.key.keysym.scancode];
						printf("*** SDL caught key release: %d\n",event.key.keysym.scancode);
						lastAsciicode=asciimap[lastScancode];
						lastScancode|=0x80;
						thePic->irqRequestAttention(1);
					}
				break;
			}
		}

	}

	delete theVideo;
	delete theDisasm;
	delete theCPU;
	delete theDebugOutput;
	delete thePit;
	delete thePic;

};

