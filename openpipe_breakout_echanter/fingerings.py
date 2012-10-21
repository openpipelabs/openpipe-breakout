#!/usr/bin/env python

#	Copyright (c) 2012 Xulio Coira <xulioc@gmail.com>. All rights reserved.
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.

# OPENPIPE FINGERINGS
# This script creates <fingerings.h> for use in OpenPipe based on
# fingering tables defined here in human readable form

# FINGER POSITION DEFINED BY A STRING LIKE "XX XXX XXXX"
# BLANKS ARE MANDATORY
# WHERE X IS:
#  C: FINGER IS CLOSED
#  O: FINGER IS OPEN
#  -: FINGER DOES NOT CARE
# FINGERS FROM LEFT TO RIGHT IN THE STRING (UP TO DOWN IN THE CHANTER):
# LEFT THUMB TOP
# LEFT THUMB BOTTOM
# LEFT INDEX
# LEFT MIDDLE
# LEFT RING
# RIGH INDEX
# RIGHT MIDDLE
# RIGHT RING
# RIGHT LITTLE

#http://www.phys.unsw.edu.au/jw/notes.html

#GAITA GALEGA
galician_mastergaita=(
	("GAITA GALEGA"), #FINGERING NAME
	(71), #BASE MIDI NOTE (THE LOWEST IN THE TABLE)
	(72), #TONIC MIDI NOTE
	(48), #DRONE MIDI NOTES
	#FINGERINGS (SEMITONES FROM BASE NOTE, (FINGERINGS,))
	(0,("-C CCC CCCC",)), # B3
	(1,("-C CCC CCCO",)), # C4
	(2,("-C CCC CCOC",)), # C#4
	(3,("-C CCC CCOO",)), # D4
	(4,("-C CCC COCO",)), # Eb4
	(5,("-C CCC COO-", "-C CCC COCC")), # E4
	(6,("-C CCC O---", "-C CCC OCC-")), # F4
	(7,("-C CCO C-OO", "-C CCO CCCC")), # F#4
	(8,("-C CCO O---", "-C CCO CCCO")), # G4
	(9,("-C COC ----",)), # Ab4
	(10,("-C COO ----",)), # A4
	(11,("-C OCO ----",)), # Bb4
	(12,("-C OOO ----", "-O CCC CCCC", "-C OCC CCCC")), # B4
    (13,("-O OOO ----", "-O CCC CCCO", "-C OCC ---O")), # C5
	(14,("-O CCC CCOC",)), # C#5
	(15,("-O CCC CCOO",)), # D5
	(16,("-O CCC COCO",)), # Eb5
	(17,("-O CCC COO-", "-O CCC COCC")), # E5
	(18,("-O CCC O---",)), # F5
	(19,("-O CCO C-OO", "-O CCO CCCC")), # F#5
    (20,("-O CCO O---", "-O CCO CCCO")), # G5
    (21,("-O COC ----",)), # Ab5
    (22,("-O COO ----",)), # A5
    (23,("-O OCO ----",)), # Bb5
    (24,("-O OCC CCCC",)), # B5
    (25,("-O OCC CCCO",)), # C6
)

# GREAT HIGHLAND BAGPIPE
#http://www.bagpipejourney.com/articles/finger_positions.shtml
great_highland_bagpipe=(
	("GREAT HIGHLAND BAGPIPE"), #FINGERING NAME
	(67), #BASE MIDI NOTE (THE LOWEST IN THE TABLE) G4
	(69), #TONIC MIDI NOTE A4
	(57), #DRONE MIDI NOTE
	#FINGERINGS (SEMITONES FROM BASE NOTE, (FINGERINGS,))
	(0,("-C CCC CCCC",)), # LOW G (G4)
	(2,("-C CCC CCCO",)), # LOW A (A4)
	(4,("-C CCC CCOO",)), # B (B4)
	(5,("-C CCC COOC", "-C CCC CO--")), # C (C5)
	(7,("-C CCC OOOC", "-C CCC OO--")), # D (D5)
	(9,("-C CCO CCCO", "-C CCO ----")), # E (E5)
	(10,("-C COO CCCO", "-C COO ----")), # F (F5)
	(12,("-C OOO CCCO", "-C OOO ----")), # HIGH G (G5)
	(14,("-O OOO CCCO", "-O OOO ----")), # HIGH A (A5)	
)

# ADD YOUR FINGERING TABLE HERE

fingerings=[]
fingerings.append(galician_mastergaita)
fingerings.append(great_highland_bagpipe)

output="""
// OPENPIPE FINGERING TABLES
// FILE AUTOMATICALLY GENERATED USING fingerings.py
// DO NOT EDIT BY HAND
"""

for index, fingering in enumerate(fingerings):
	output+="""
	
// %s
const char fingering_name_%i[]={"%s"};
const unsigned long fingering_table_%i[]={
""" % (fingering[0], index, fingering[0], index)

	output+= "\t"
	output+= "%s," % fingering[1] # BASE NOTE
	output+= "%s," % fingering[2] # TONIC NOTE
	output+= "%s," % fingering[3] # DRONE NOTE
	output+= "\r\n\t"
	
	for item in fingering[4:]:
		for fingers in item[1]:
			#"-C CCC CCCC -"
			f=0x0		# FINGER VALUE
			m=0xFFFF	# MASK VALUE
                        for i,pos in enumerate((10,9,8,7,5,4,3,1)):
				if fingers[pos]=='C':
					f+=2**i
				if fingers[pos]=='-':
					m-=2**i
			value=(1<<31)+(f<<16)+m
			output+= "0x%08X, " % value
		value=int(item[0])<<24
		output+= "0x%08X," % value
		output+= "\r\n\t"
	output+= "0xFFFFFFFF\r\n};" #TABLE END
	

output+= "\r\n\r\n"
for i in range(len(fingerings)):
	output+="#define FINGERING_%s %i" %(fingerings[i][0].replace (" ", "_"), i)
	output+= "\r\n"

output+="""

typedef struct{
	char* name;
	unsigned long* table;
}fingering_t;

#define TOTAL_FINGERINGS %i
const fingering_t fingerings[TOTAL_FINGERINGS]={
""" % (len(fingerings))


	

for i in range(len(fingerings)):
	output+="\t{(char*)fingering_name_%i, (unsigned long*)fingering_table_%i},\r\n" % (i,i)

output+="};\r\n"

print output

