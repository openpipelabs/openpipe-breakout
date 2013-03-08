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

import math
import sys

BITS=8
SAMPLE_RATE=44100
SIGNED=False

# SINUSOIDS
sinusoids=(
	("SINUSOIDS"), #INSTRUMENT NAME
	(67, 392, 1.0, (1,)), #LOW G
	(69, 440, 1.0, (1,)),
	(71, 493, 1.0, (1,)),
	(72, 523, 1.0, (1,)),
	(74, 587, 1.0, (1,)),
	(76, 659, 1.0, (1,)),
	(77, 698, 1.0, (1,)),
	(79, 783, 1.0, (1,)),
	(81, 880, 1.0, (1,)),
)

#GAITA GALEGA
gaita_galega=(
	("GAITA_GALEGA"), #INSTRUMENT NAME
	(48, 130.8128, 1, (1.0, 0.5131, 0.9251, 0.7995, 0.9317, 0.3742, 0.0386, 0.1261, 0.0574, 0.0361, 0.0332, 0.0145, 0.0112, 0.0109, 0.0049, 0.0094, 0.013, 0.0317, 0.0137, 0.0544)),
	(60, 261.6256, 1, (0.4855, 0.5149, 0.4062, 0.2259, 0.2127, 1.0, 0.0408, 0.1016, 0.0239, 0.0462, 0.2333, 0.1041, 0.0934, 0.009, 0.0564, 0.0171, 0.1189, 0.1485, 0.0447, 0.0382)),
	(71, 493.8833, 1, (0.3976, 0.6409, 0.4899, 1.0, 0.3137, 0.1548, 0.2085, 0.2972, 0.1409, 0.0243, 0.0411, 0.0668, 0.0734, 0.0875, 0.0509, 0.0428, 0.0126, 0.0476, 0.0782, 0.0916)),
	(72, 523.2511, 1, (0.4623, 0.8685, 0.7808, 1.0, 0.681, 0.1535, 0.3219, 0.503, 0.3814, 0.038, 0.1563, 0.1045, 0.073, 0.0265, 0.0269, 0.0263, 0.055, 0.1148, 0.2005, 0.1315)),
	(73, 554.3700, 1, (0.4623, 0.8685, 0.7808, 1.0, 0.681, 0.1535, 0.3219, 0.503, 0.3814, 0.038, 0.1563, 0.1045, 0.073, 0.0265, 0.0269, 0.0263, 0.055, 0.1148, 0.2005, 0.1315)),
	(74, 587.3295, 1, (0.5589, 0.6633, 0.7047, 1.0, 0.3202, 0.1594, 0.6539, 0.1778, 0.1456, 0.0314, 0.0546, 0.0649, 0.0438, 0.0161, 0.0325, 0.0187, 0.1526, 0.1778, 0.0968, 0.078)),
	(75, 622.2500, 1, (0.5589, 0.6633, 0.7047, 1.0, 0.3202, 0.1594, 0.6539, 0.1778, 0.1456, 0.0314, 0.0546, 0.0649, 0.0438, 0.0161, 0.0325, 0.0187, 0.1526, 0.1778, 0.0968, 0.078)),
	(76, 659.2551, 1, (0.3047, 0.4057, 0.7287, 1.0, 0.1276, 0.556, 0.103, 0.0657, 0.0437, 0.0436, 0.0162, 0.0078, 0.0182, 0.0256, 0.0649, 0.0721, 0.0189, 0.0363, 0.0278, 0.0277)),
	(77, 698.4565, 1, (0.397, 0.6125, 1.0, 0.2904, 0.3445, 0.3505, 0.3175, 0.1612, 0.1339, 0.0623, 0.044, 0.039, 0.0386, 0.0172, 0.1171, 0.076, 0.1353, 0.0821, 0.0826, 0.0921)),
	(78, 739.9900, 1, (0.397, 0.6125, 1.0, 0.2904, 0.3445, 0.3505, 0.3175, 0.1612, 0.1339, 0.0623, 0.044, 0.039, 0.0386, 0.0172, 0.1171, 0.076, 0.1353, 0.0821, 0.0826, 0.0921)),
	(79, 783.9909, 1, (0.1833, 0.3204, 1.0, 0.6586, 0.3577, 0.0487, 0.0459, 0.0309, 0.0359, 0.0279, 0.0396, 0.028, 0.0578, 0.0866, 0.1601, 0.1364, 0.0622, 0.0372, 0.0168, 0.0099)),
	(80, 830.6100, 1, (0.1833, 0.3204, 1.0, 0.6586, 0.3577, 0.0487, 0.0459, 0.0309, 0.0359, 0.0279, 0.0396, 0.028, 0.0578, 0.0866, 0.1601, 0.1364, 0.0622, 0.0372, 0.0168, 0.0099)),
	(81, 880.0000, 1, (0.2423, 0.4105, 1.0, 0.196, 0.246, 0.2087, 0.1997, 0.0206, 0.0279, 0.0514, 0.0892, 0.1065, 0.1298, 0.1729, 0.0758, 0.0443, 0.0366, 0.0155, 0.012, 0.0102)),
	(82, 932.3300, 1, (0.2423, 0.4105, 1.0, 0.196, 0.246, 0.2087, 0.1997, 0.0206, 0.0279, 0.0514, 0.0892, 0.1065, 0.1298, 0.1729, 0.0758, 0.0443, 0.0366, 0.0155, 0.012, 0.0102)),
	(83, 987.7666, 1, (0.336, 0.4945, 1.0, 0.3454, 0.2002, 0.0763, 0.0705, 0.0253, 0.0754, 0.0422, 0.2107, 0.3412, 0.2618, 0.1425, 0.0864, 0.0299, 0.0223, 0.0375, 0.0464, 0.0596)),
	(84, 1046.5023, 1, (0.4391, 1.0, 0.4572, 0.2788, 0.0629, 0.1973, 0.0596, 0.0133, 0.0569, 0.0371, 0.0557, 0.0333, 0.0205, 0.0279, 0.0186, 0.006, 0.0058, 0.0051, 0.0064, 0.0035)),	
)

#GREAT HIGHLAND BAGPIPE
ghb=(
	("GHB"), #INSTRUMENT NAME
	(67, 414, 1.00, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #LOW G
	(69, 466, 1.00, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #LOW A
	(71, 524, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #B
	(72, 559, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #CNAT
	(73, 583, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #C
	(74, 621, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #D
	(76, 699, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #E
	(77, 746, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #FNAT
	(78, 777, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #F
	(79, 839, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #HIGH G
	(81, 932, 0.92, (1.12202, 1.00577, 1.03039, 1.01508, 1.06660, 1.01508, 1.02920, 1.00346, 1.00809)), #HIGH A
)

instruments=[]
#instruments.append(sinusoids)
instruments.append(gaita_galega)
instruments.append(ghb)

generated=[]
for instrument in instruments:
	samples=[]
	#print instrument[0]
	for note in instrument[1:]:
		freq=note[1]
		total_samples=int(SAMPLE_RATE/freq)
		#sinusoid=[int((math.sin(2.0*math.pi*freq*(x/float(SAMPLE_RATE)))) for x in range(total_samples)]	
		partials=[]
		for i,partial in enumerate(note[3]):
			partial_sinusoid=[(math.sin(2.0*math.pi*freq*(i+1)*(x/float(SAMPLE_RATE))))*partial for x in range((total_samples))]
			partials.append(partial_sinusoid)
		
		#MIX PARTIALS
		loop=[]
		for i in range(total_samples):
			value=0
			for item in partials:
				value+=item[i]
			loop.append(value)
		
		#NORMALIZE
		max_sample=max(max(loop),abs(min(loop)))
		#print max_sample
		if SIGNED==False:
			factor=((((2**BITS))/2)-1)/max_sample
		else:
			factor=((((2**BITS))/2)-1)/max_sample
		normalized_loop=[int(x*factor) for x in loop]
		#print max(normalized_loop)
		#print min(normalized_loop)
		if SIGNED==False:
			normalized_loop=[x+((((2**BITS))/2)-1) for x in normalized_loop]
			
		#print max(normalized_loop)
		#print min(normalized_loop)
		#sys.exit()
	
		sample_name="instrument_"+instrument[0]+"_note_"+str(note[0])
		sample=(sample_name, note[0], normalized_loop)
		samples.append(sample)
	generated.append((instrument[0], samples))


#print generated

output="""
// OPENPIPE SAMPLES
// FILE AUTOMATICALLY GENERATED USING samples.py
// DO NOT EDIT BY HAND

typedef struct{
  int16_t note;
  int16_t len;
  char* sample;
}sample_t;

typedef struct{
  char* name;
  sample_t* samples;
}instrument_t;

"""

if BITS>8:
	sample_type="prog_int16_t"
else:
	sample_type="prog_uint8_t"
	
for item in generated:
	#print item[0]
	for sample in item[1]:
		#print sample[0]
		output+="PROGMEM %s %s [] = {" % (sample_type,sample[0])
		for value in sample[2]:
			output+=str(value)+","
		output+="};\r\n"
		
for item in generated:
	output += "\r\nconst sample_t instrument_%s_samples[]={\r\n" % item[0]
	for sample in item[1]:
		output+="\t{%i, %i, (char*)%s},\r\n" % (sample[1], len(sample[2]), sample[0])
	output += "\t{0xFF, 0,0} // END\r\n};"

output += "\r\n"
for i,item in enumerate(generated):
	output+= "#define INSTRUMENT_%s %i\r\n" % (item[0], i)

output += "\r\nconst instrument_t instruments[]={\r\n"
for item in generated:
	output+= "{(char*)\"%s\", (sample_t*)instrument_%s_samples},\r\n" % (item[0], item[0]) 
output += "};"

print output
sys.exit()
