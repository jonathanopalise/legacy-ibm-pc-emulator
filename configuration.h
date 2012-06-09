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

#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

#include"legacytypes.h"

#define CONFIGURATION_INITIAL_VIDEO_MODE_EGA_VGA 0
#define CONFIGURATION_INITIAL_VIDEO_MODE_40_25_CGA 1
#define CONFIGURATION_INITIAL_VIDEO_MODE_80_25_CGA 2
#define CONFIGURATION_INITIAL_VIDEO_MODE_MONOCHROME 3

#define CONFIGURATION_NOT_INSTALLED 0
#define CONFIGURATION_INSTALLED 1

class CConfiguration {
public:
	CConfiguration();
	uint16 getConfigurationuint16();

	void set8087(uint16 has8087);
	void setMouse(uint16 hasMouse);
	void setInitialVideoMode(uint16 initialVideoMode);
	void setNumDisketteDrives(uint16 numDisketteDrives);
	void setNumSerialAdapters(uint16 numSerialAdapters);
	void setGameAdapter(uint16 hasGameAdapter);
	void setNumParallelAdapters(uint16 numParallelAdapters);
private:
	uint16 configuration;
};

#endif
