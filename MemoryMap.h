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

#ifndef __MEMORYMAP_H
#define __MEMORYMAP_H

#define MM_FLOPPY_DRIVE_MOTOR_STATUS_BYTE (MAKE_ABSOLUTE_ADDRESS(0x0000,0x0441))

#define MM_MACHINE_CONFIGURATION (MAKE_ABSOLUTE_ADDRESS(0x0000,0x0410))
#define MM_CURRENT_VIDEO_MODE_BYTE (MAKE_ABSOLUTE_ADDRESS(0x0000,0x0449))
#define MM_TEXT_ROW_MAX_CHARS_BYTE (MAKE_ABSOLUTE_ADDRESS(0x0000,0x044A))
#define MM_BYTES_NEEDED_TO_DISPLAY_SCREEN_WORD (MAKE_ABSOLUTE_ADDRESS(0x0000,0x044C))
#define MM_CURRENT_DISPLAY_PAGE_OFFSET_WORD (MAKE_ABSOLUTE_ADDRESS(0x0000,0x044E))
#define MM_DISPLAY_PAGE_CURSOR_LOCATIONS_WORD8 (MAKE_ABSOLUTE_ADDRESS(0x0000,0x0450))
#define MM_CURRENT_CURSOR_SIZE_BYTE2 (MAKE_ABSOLUTE_ADDRESS(0x0000,0x0460))
#define MM_CURRENT_DISPLAY_PAGE_BYTE (MAKE_ABSOLUTE_ADDRESS(0x0000,0x0462))

#define MM_CONTIGUOUS_MEMORY_SIZE_WORD (MAKE_ABSOLUTE_ADDRESS(0x0040,0x0013))


#endif
