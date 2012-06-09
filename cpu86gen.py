#
#    Legacy (trunk) - a portable emulator of the 8086-based IBM PC series of computers
#    Copyright (C) 2005 Jonathan Thomas
#    Contributions and fixes to CPU emulation by Jim Shaw
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#



print 'void Cpu86::decodemodrmbyte() {'
print '\tmodrmbyte=getmemuint8(cs,ip+1);'
print '\tswitch(modrmbyte) {'

for modrmbyte in range(0,0xc0):
	print '\t\tcase '+hex(modrmbyte)+':'

	print '\t\t\tregPointer=reglookup[regmode+'+`((modrmbyte>>3)&7)`+'];'
	#print '\t\t\tregPointer=reglookup[regmode+((modrmbyte>>3)&7)];'

	instructionSize=2
	effectiveAddressOffsetAdder=''

	modrmbyteand7=modrmbyte&7
	if modrmbyteand7==0:
		effectiveAddressOffsetAdder='bx+si'
	elif modrmbyteand7==1:
		effectiveAddressOffsetAdder='bx+di'
	elif modrmbyteand7==2:
		effectiveAddressOffsetAdder='bp+si'
	elif modrmbyteand7==3:
		effectiveAddressOffsetAdder='bp+di'
	elif modrmbyteand7==4:
		effectiveAddressOffsetAdder='si'
	elif modrmbyteand7==5:
		effectiveAddressOffsetAdder='di'
	elif modrmbyteand7==6:
		if modrmbyte>0x3f:
			effectiveAddressOffsetAdder='bp'
		else:
			effectiveAddressOffsetAdder='getmemuint16(cs,(ip)+2)'
			instructionSize=4
	elif modrmbyteand7==7:
		effectiveAddressOffsetAdder='bx'

	test=modrmbyte&0xc0
	if test==0x80:
		effectiveAddressOffsetAdder=effectiveAddressOffsetAdder+'+getmemuint16(cs,(ip)+2)'
		instructionSize=4
	elif test==0x40:
		effectiveAddressOffsetAdder=effectiveAddressOffsetAdder+'+(sint8)getmemuint8(cs,(ip)+2)'
		instructionSize=3

	#print '\t\t\teffectiveAddressOffset='+effectiveAddressOffsetAdder+';'
	if modrmbyteand7==2 or modrmbyteand7==3 or (modrmbyte>0x3f and modrmbyteand7==6):
		print '\t\t\teffectiveAddressPointer=memory+((segmentDefault ? ss : *activeSegment )*16)+('+effectiveAddressOffsetAdder+');'
		#print '\t\t\teffectiveAddressPointer=memory+((segmentDefault ? ss : (*activeSegment))*16)+effectiveAddressOffset;'
	else:
		print '\t\t\teffectiveAddressPointer=memory+((*activeSegment)*16)+('+effectiveAddressOffsetAdder+');'

	print '\t\t\tinstructionSize='+`instructionSize`+';'
	print '\t\t\tbreak;'

print '\t\tdefault:'
print '\t\t\tregPointer=reglookup[regmode+((modrmbyte>>3)&7)];'
print '\t\t\tinstructionSize=2;'
print '\t\t\teffectiveAddressPointer=reglookup[earegmode+(modrmbyte&7)];'
print '\t}'
print '}'
