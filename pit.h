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

#ifndef __PIT_H
#define __PIT_H

#include "legacytypes.h"

class CPit {

private:
	uint8 counterToReceive;
	uint8 portState;
	uint16 *counterValue;
	uint16 counterLatchValue;
public:
	CPit();
	void writeControlByte(uint8 value);
	void writeCounterValue(uint8 counter,uint8 value);
	uint16 getCounterValue(uint8 counter);
	~CPit();

};

#endif
