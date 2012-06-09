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

#ifndef __DISKIMAGE_H
#define __DISKIMAGE_H

#define DISKIMAGE_LOADINONEGO

#include<stdio.h>
#include"legacytypes.h"

#define DISKIMAGE_NO_ERROR 0

// constants for use with loadData() and saveData()

#define DISKIMAGE_MEDIA_NOT_FOUND 1
#define DISKIMAGE_SECTOR_NOT_FOUND 4
#define DISKIMAGE_SEEK_ERROR 3

// constants for use with insertMedia()

#define DISKIMAGE_FILE_NOT_FOUND 1
#define DISKIMAGE_INCORRECT_SIZE 2

class CDiskImage {
public:
	CDiskImage(uint16 heads,uint16 tracks,uint16 sectors);
	uint16 getStartSector(uint16 head, uint16 track, uint16 sector);
	uint8 insertMedia(char *filename);
	void ejectMedia();
	uint16 loadData(char *buffer,uint16 head,uint16 track,uint16 sector,uint16 numSectors);
	uint16 saveData(uint8 *buffer,uint16 head,uint16 track,uint16 sector,uint16 numSectors);
	uint8 getError();
	uint16 heads;
	uint16 tracks;
	uint16 sectors;
	uint16 totalSectors;
	uint16 bytesPerSector;
private:
	uint8 error;
	FILE *image;

#ifdef DISKIMAGE_LOADINONEGO

	uint8 *memoryImage;

#endif
};

#endif
