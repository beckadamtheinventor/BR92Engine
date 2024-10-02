import os, sys, struct

CHUNKSIZE = 10

def parseColor(s):
	if not s.startswith("0x"):
		raise ValueError("Wrong color format")
	return [int(s[2:4], 16), int(s[4:6], 16), int(s[6:8], 16), int(s[8:10], 16)]

def writeSection(binary, header, data):
	binary.extend([ord(c) for c in header])
	binary.extend(list(len(data).to_bytes(4, 'little')))
	binary.extend(data)

def writeTileSection(binary, data, msx, msy, x, y, z, sx=0, sz=0, ex=-1, ez=-1, scl=1):
	shouldWrite = False
	for row in data[sz:ez]:
		for col in row[sx:ex]:
			if col != 2:
				shouldWrite = True
	if not shouldWrite:
		return
	o = []
	o.extend(list(x.to_bytes(4, 'little', signed=True)))
	o.extend(list(y.to_bytes(4, 'little', signed=True)))
	o.extend(list(z.to_bytes(4, 'little', signed=True)))
	o.append(msx)
	o.append(msy)
	for row in data[sz:ez]:
		for col in row[sx:ex]:
			tile = col - 2
			o.extend(tile.to_bytes(2, 'little'))
	writeSection(binary, "TILE", o)

def writeFogSection(binary, color, fogmin, fogmax):
	o = color
	o.extend(list(struct.pack("ff", fogmin, fogmax)))
	writeSection(binary, "FOGC", o)

def writeLightSection(binary, lightlevel):
	writeSection(binary, "LMUL", list(struct.pack("f", lightlevel)))

def writeEntitySection(binary, etype, pos, rot):
	o = list(etype.to_bytes(2, 'little'))
	o.extend(list(struct.pack("ffff", pos[0], pos[1], pos[2], rot)))
	writeSection(binary, "ENTT", o)

if __name__=='__main__':
	if len(sys.argv) >= 4:
		try:
			with open(sys.argv[1]) as f:
				fdata = f.read().split("\n")
		except Exception as e:
			print(f"Error loading level csv \"{sys.argv[1]}\"\nOriginal Error: {str(e)}")
			exit(1)
	else:
		print(f"Usage: {sys.argv[0]} level.csv level.dat x,y,z [-a][--fog color,min,max][--light multiplier][--entity type,x,y,z,r]\nNote: color must be formatted as a hex color")
		exit(0)
	
	data = []
	for line in fdata:
		if len(line):
			data.append([int(c) for c in line.split(",")])
	
	data.reverse()
	
	try:
		x,y,z = [int(c) for c in sys.argv[3].split(",")]
	except Exception as e:
		print(f"Error in arguments. x,y,z must be three integers separated by commas\nOriginal Error: {str(e)}")
		exit(1)

	binary = []
	width = len(data)
	height = len(data[0])
	if width > CHUNKSIZE or height > CHUNKSIZE:
		for zz in range(0, height, CHUNKSIZE):
			for xx in range(0, width, CHUNKSIZE):
				writeTileSection(binary, data, CHUNKSIZE, CHUNKSIZE, x+xx, y, z+zz, xx, zz, min(xx+CHUNKSIZE, width), min(zz+CHUNKSIZE, height))
	else:
		writeTileSection(binary, data, width, height, x, y, z)

	mode = "wb"
	fogcolor = None
	lightlevel = None
	for i in range(4, len(sys.argv)):
		if sys.argv[i] == "-a" and os.path.exists(sys.argv[2]):
			mode = "ab"
		elif sys.argv[i] == "--fog" and i+1<len(sys.argv):
			try:
				fogcolor, fogmin, fogmax = sys.argv[i+1].split(",")
				fogcolor = parseColor(fogcolor)
				fogmin = float(fogmin)
				fogmax = float(fogmax)
			except Exception as e:
				print(f"Error in arguments. Fog color color,min,max must be a hex color and two floats separated by commas")
				exit(1)
		elif sys.argv[i] == "--light" and i+1<len(sys.argv):
			try:
				lightlevel = float(sys.argv[i+1])
			except Exception as e:
				print(f"Error in arguments. Light level must be a single floating point number")
				exit(1)
		elif sys.argv[i] == "--entity" and i+1<len(sys.argv):
			try:
				t,x,y,z,r = sys.argv[i+1].split(",")
				t = int(t)
				x = float(x)
				y = float(y)
				z = float(z)
				r = float(r)
				writeEntitySection(binary, t, [x, y, z], r)
			except Exception as e:
				print(f"Error in arguments. Entity placement must be an integer followed by four floats x,y,z and rotation")
				exit(1)

	if fogcolor is not None:
		writeFogSection(binary, fogcolor, fogmin, fogmax)

	if lightlevel is not None:
		writeLightSection(binary, lightlevel)

	try:
		with open(sys.argv[2], mode) as f:
			f.write(bytes(binary))
	except Exception as e:
		print(f"Error writing output file.\nOriginal Error: {str(e)}")
		exit(1)
