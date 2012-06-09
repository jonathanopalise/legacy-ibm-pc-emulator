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

#include"configuration.h"

// not working properly

CConfiguration::CConfiguration() {
	configuration=0;
}

void CConfiguration::set8087(uint16 has8087) {
	configuration&=(configuration&(~(1<<1)))|((has8087&1)<<1);
}

void CConfiguration::setMouse(uint16 hasMouse) {
	configuration&=(configuration&(~(1<<2)))|((hasMouse&1)<<2);
}

void CConfiguration::setInitialVideoMode(uint16 initialVideoMode) {
	configuration&=(configuration&(~(3<<4)))|((initialVideoMode&3)<<4);
}

void CConfiguration::setNumDisketteDrives(uint16 numDisketteDrives) {
	configuration&=(configuration&(~(3<<6)))|(((numDisketteDrives-1)&3)<<6);
}

void CConfiguration::setNumSerialAdapters(uint16 numSerialAdapters) {
	configuration&=(configuration&(~(7<<9)))|((numSerialAdapters&7)<<9);
}

void CConfiguration::setGameAdapter(uint16 hasGameAdapter) {
	configuration&=(configuration&(~(1<<12)))|((hasGameAdapter&1)<<12);
}

void CConfiguration::setNumParallelAdapters(uint16 numParallelAdapters) {
	configuration&=(configuration&(~(3<<14)))|((numParallelAdapters&3)<<14);
}

uint16 CConfiguration::getConfigurationuint16() {
	return(configuration);
}
