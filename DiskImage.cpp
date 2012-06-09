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

#include"DiskImage.h"
#include"DebugOutput.h"
#include <memory.h>

extern CDebugOutput *theDebugOutput;

#ifdef DISKIMAGE_LOADINONEGO

CDiskImage::CDiskImage(uint16 heads,uint16 tracks,uint16 sectors) {	
	this->heads=heads;
	this->tracks=tracks;
	this->sectors=sectors;
	this->totalSectors=heads*tracks*sectors;
	this->bytesPerSector=512;
}

uint8 CDiskImage::insertMedia(char *filename) {

	image=fopen(filename,"rb+");
	if (image!=NULL) {
		fseek(image,0,SEEK_END);
		if (ftell(image)==totalSectors*bytesPerSector) {
			memoryImage=new uint8[totalSectors*bytesPerSector];
			fseek(image,0,SEEK_SET);
			fread(memoryImage,bytesPerSector,totalSectors,image);
			fclose(image);
			return(DISKIMAGE_NO_ERROR);
		} else {
			fclose(image);
			return(DISKIMAGE_INCORRECT_SIZE);
		}
	} else {
		return(DISKIMAGE_FILE_NOT_FOUND);
	}

}

void CDiskImage::ejectMedia() {
	if (memoryImage!=NULL) {
		delete []memoryImage;
	}
}

uint16 CDiskImage::getStartSector(uint16 head, uint16 track, uint16 sector) {
	return(((track*heads+head)*sectors)+sector-1);
}

uint16 CDiskImage::loadData(char *buffer,uint16 head,uint16 track,uint16 sector,uint16 numSectors) {

	//printf("heads: %d, tracks: %d, sectors: %d\n",heads,tracks,sectors);
	printf("loadData(%d,%d,%d,%d) = %d %x\n",head,track,sector,numSectors,getStartSector(head,track,sector),getStartSector(head,track,sector));

	// head starts at 0
	// track aka cylinder starts at 0
	// sector starts at 1

	//sprintf(theDebugOutput->getBuffer(),"loading from floppy - head %d, track %d, sector %d, numsectors %d\n",head,track,sector,numSectors);
	//theDebugOutput->writeMessage();

	if (memoryImage!=NULL) {
		//printf("head: %d, heads: %d\n",head,heads);
		//printf("track: %d, tracks: %d\n",track,tracks);
		//printf("sector: %d, sectors: %d\n",sector,sectors);

		if ((head<=heads) && (track<=tracks) && (sector<=sectors)) {
			uint16 startSector=getStartSector(head,track,sector);
			if (numSectors>(totalSectors-startSector)) {
				numSectors=totalSectors-startSector;
			}
			if (sector>sectors) {
				printf("ALLO\n");
				error=DISKIMAGE_SECTOR_NOT_FOUND;
				return(0);
			}
			error=DISKIMAGE_NO_ERROR;
			//sprintf(theDebugOutput->getBuffer(),"DISKIMAGE_NO_ERROR\n");
			//theDebugOutput->writeMessage();

			//sprintf(theDebugOutput->getBuffer(),"LoadData copying %d bytes from %x to %x\n",numSectors*bytesPerSector,(uint32)memoryImage+(startSector*bytesPerSector),(uint32)buffer);
			//theDebugOutput->writeMessage();
			
			memcpy(buffer,&memoryImage[startSector*bytesPerSector],numSectors*bytesPerSector);
			return(numSectors);
		} else {
			printf("OOWS THAT\n");
			printf("DISKIMAGE_SECTOR_NOT_FOUND\n");
			//theDebugOutput->writeMessage();
			error=DISKIMAGE_SECTOR_NOT_FOUND;
			return(0);
		}
	} else {
		//sprintf(theDebugOutput->getBuffer(),"DISKIMAGE_MEDIA_NOT_FOUND\n");
		//theDebugOutput->writeMessage();
		error=DISKIMAGE_MEDIA_NOT_FOUND;
		return(0);
	}
}



uint16 CDiskImage::saveData(uint8 *buffer,uint16 head,uint16 track,uint16 sector,uint16 numSectors) {

	printf("head: %d, heads: %d\n",head,heads);
	printf("track: %d, tracks: %d\n",track,tracks);
	printf("sector: %d, sectors: %d\n",sector,sectors);

	/*if (image!=NULL) {
		if ((head<=heads) && (track<=tracks) && (sector<=sectors)) {
			uint16 startSector=getStartSector(head,track,sector);
			if (numSectors>(totalSectors-startSector)) {
				numSectors=totalSectors-startSector;
			}
			fseek(image,startSector*bytesPerSector,SEEK_SET);
			error=DISKIMAGE_NO_ERROR;
			return(fwrite(buffer,bytesPerSector,numSectors,image));
			memcpy(memoryImage,buffer,bytesPerSector*numSectors);
			return(bytesPerSector*numSectors);
		} else {
			error=DISKIMAGE_SECTOR_NOT_FOUND;
			return(0);
		}
	} else {
		error=DISKIMAGE_MEDIA_NOT_FOUND;
		return(0);
	}*/

    return 0;
}

uint8 CDiskImage::getError() {
	return(error);
}

#else

CDiskImage::CDiskImage(uint16 heads,uint16 tracks,uint16 sectors) {
	this->heads=heads;
	this->tracks=tracks;
	this->sectors=sectors;
	this->totalSectors=heads*tracks*sectors;
}

uint8 CDiskImage::insertMedia(char *filename) {
	// ensure that file is correct size
	
	image=fopen(filename,"rb+");
	if (image!=NULL) {
		fseek(image,0,SEEK_END);
		if (ftell(image)==totalSectors*bytesPerSector) {
			return(DISKIMAGE_NO_ERROR);
		} else {
			fclose(image);
			return(DISKIMAGE_INCORRECT_SIZE);
		}
	} else {
		printf("image file not found!\n");
		return(DISKIMAGE_FILE_NOT_FOUND);
	}
}

void CDiskImage::ejectMedia() {
	if (image!=NULL) {
		fclose(image);
	}
}

uint16 CDiskImage::getStartSector(uint16 head, uint16 track, uint16 sector) {
	// convert a CHS address to an LBA address
	return(((track*heads+head)*sectors)+sector-1);

	//return((sector-1)+(head*sectors)+(track*(heads+1)*sectors));

}

uint16 CDiskImage::loadData(char *buffer,uint16 head,uint16 track,uint16 sector,uint16 numSectors) {
	printf("eloadData(%d,%d,%d,%d) = %d %x\n",head,track,sector,numSectors,getStartSector(head,track,sector),getStartSector(head,track,sector));

	sprintf(theDebugOutput->getBuffer(),"loading from floppy - head %d, track %d, sector %d, numsectors %d\n",head,track,sector,numSectors);
	theDebugOutput->writeMessage();

	if (image!=NULL) {
		if ((head<=heads) && (track<=tracks) && (sector<=sectors)) {
			uint16 startSector=getStartSector(head,track,sector);
			if (numSectors>(totalSectors-startSector)) {
				numSectors=totalSectors-startSector;
			}
			//printf("fseek(%d) %d\n",startSector,startSector*bytesPerSector);
			//getchar();
			fseek(image,startSector*bytesPerSector,SEEK_SET);
			error=DISKIMAGE_NO_ERROR;
			sprintf(theDebugOutput->getBuffer(),"DISKIMAGE_NO_ERROR\n");
			theDebugOutput->writeMessage();

			return(fread(buffer,bytesPerSector,numSectors,image));
		} else {
			sprintf(theDebugOutput->getBuffer(),"DISKIMAGE_SECTOR_NOT_FOUND\n");
			theDebugOutput->writeMessage();
			error=DISKIMAGE_SECTOR_NOT_FOUND;
			return(0);
		}
	} else {
		sprintf(theDebugOutput->getBuffer(),"DISKIMAGE_MEDIA_NOT_FOUND\n");
		theDebugOutput->writeMessage();
		error=DISKIMAGE_MEDIA_NOT_FOUND;
		return(0);
	}
}

uint16 CDiskImage::saveData(char *buffer,uint16 head,uint16 track,uint16 sector,uint16 numSectors) {
	if (image!=NULL) {
		if ((head<=heads) && (track<=tracks) && (sector<=sectors)) {
			uint16 startSector=getStartSector(head,track,sector);
			if (numSectors>(totalSectors-startSector)) {
				numSectors=totalSectors-startSector;
			}
			fseek(image,startSector*bytesPerSector,SEEK_SET);
			error=DISKIMAGE_NO_ERROR;
			return(fwrite(buffer,bytesPerSector,numSectors,image));
		} else {
			error=DISKIMAGE_SECTOR_NOT_FOUND;
			return(0);
		}
	} else {
		error=DISKIMAGE_MEDIA_NOT_FOUND;
		return(0);
	}
}

uint8 CDiskImage::getError() {
	return(error);
}

#endif
