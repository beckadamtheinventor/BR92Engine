import os, sys, struct

CHUNKSIZE = 10

def parseColor(s):
	if not s.startswith("0x"):
		raise ValueError("Wrong color format")
	return [int(s[2:4], 16), int(s[4:6], 16), int(s[6:8], 16), int(s[8:10], 16)]

def writeTileSection(binary, data, msx, msy, x, y, z, sx=0, sz=0, ex=-1, ez=-1):
	shouldWrite = False
	for row in data[sz:ez]:
		for col in row[sx:ex]:
			if col != 2:
				shouldWrite = True
	if not shouldWrite:
		return
	binary.extend([ord(c) for c in "TILE"])
	binary.extend(list(x.to_bytes(4, 'little', signed=True)))
	binary.extend(list(y.to_bytes(4, 'little', signed=True)))
	binary.extend(list(z.to_bytes(4, 'little', signed=True)))
	binary.append(msx)
	binary.append(msy)
	for row in data[sz:ez]:
		for col in row[sx:ex]:
			tile = col - 2
			binary.extend(tile.to_bytes(2, 'little'))

def writeFogSection(binary, color, fogmin, fogmax):
	binary.extend([ord(c) for c in "FOGC"])
	binary.extend(color)
	binary.extend(list(struct.pack("ff", fogmin, fogmax)))

if __name__=='__main__':
	if len(sys.argv) >= 4:
		try:
			with open(sys.argv[1]) as f:
				fdata = f.read().split("\n")
		except Exception as e:
			print(f"Error loading level csv \"{sys.argv[1]}\"\nOriginal Error: {str(e)}")
			exit(1)
	else:
		print(f"Usage: {sys.argv[0]} level.csv level.dat x,y,z [-a][--fog color,min,max]\nNote: color must be formatted as a hex color")
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
	
	if fogcolor is not None:
		writeFogSection(binary, fogcolor, fogmin, fogmax)

	try:
		with open(sys.argv[2], mode) as f:
			f.write(bytes(binary))
	except Exception as e:
		print(f"Error writing output file.\nOriginal Error: {str(e)}")
		exit(1)
