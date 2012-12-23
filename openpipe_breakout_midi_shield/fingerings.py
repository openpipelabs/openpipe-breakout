#!/usr/bin/env python

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
	(68), #BASE MIDI NOTE (THE LOWEST IN THE TABLE)
	(70), #TONIC MIDI NOTE
	(57), #DRONE MIDI NOTE
	#FINGERINGS (SEMITONES FROM BASE NOTE, (FINGERINGS,))
	(0,("-C CCC CCCC",)), # LOW G
	(2,("-C CCC CCCO",)), # LOW A
	(4,("-C CCC CCOO",)), # B
	(6,("-C CCC COOC", "-C CCC CO--")), # C
	(7,("-C CCC OOOC", "-C CCC OO--")), # D
	(9,("-C CCO CCCO", "-C CCO ----")), # E
	(11,("-C COO CCCO", "-C CO- ----")), # F
	(12,("-C OOO CCCO", "-C O-- ----")), # HIGH G
	(14,("-O OOO CCCO", "-O --- ----")), # HIGH A	
)

# UILLEANN PIPE
uilleann_pipe=(
	("UILLEANN PIPE"), #FINGERING NAME
	(62), #BASE MIDI NOTE (THE LOWEST IN THE TABLE)
	(62), #TONIC MIDI NOTE
	(0), #DRONE MIDI NOTE
	#FINGERINGS (SEMITONES FROM BASE NOTE, (FINGERINGS,))
	(0,("-C CCC CCCC",)), # D  
	(2,("-C CCC CCOO",)), # E 
	(4,("-C CCC COCC",)), # F#
	(5,("-C CCC OOCC",)), # G 
	(7,("-C CCO CCCC",)), # A
	(9,("-C COO CCCC",)), # B
	(10,("-C OCC COCC",)), # C
	(11,("-C OCC CCCC",)), # C#
	(12,("-O CCC CCCC",)), # D
)

# SACKPIPA
#http://olle.gallmo.se/sackpipa/playing.php?lang=en
sackpipa=(
	("SACKPIPA"), #FINGERING NAME
	(62), #BASE MIDI NOTE (THE LOWEST IN THE TABLE)
	(63), #TONIC MIDI NOTE
	(45), #DRONE MIDI NOTE
	#FINGERINGS (SEMITONES FROM BASE NOTE, (FINGERINGS,))
	(0,("-C CCC CCCC",)), #D
	(2,("-C CCC CCCO",)), #E
	(4,("-C CCC CCOO",)), #F#
	(5,("-C CCC COC-",)), #G
	(6,("-C CCC CO--",)), #G#
	(7,("-C CCC ----",)), #A
	(9,("-C CCO ----",)), #B
	(10,("-C CO- ----",)), #C
	(11,("-C OCO ----",)), #C#
	(13,("-C O-- ----",)), #D
	(15,("-O O-- ----",)), #E
)

# ADD YOUR FINGERING TABLE HERE

fingerings=[]
fingerings.append(great_highland_bagpipe)
fingerings.append(galician_mastergaita)
fingerings.append(uilleann_pipe)
fingerings.append(sackpipa)

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

