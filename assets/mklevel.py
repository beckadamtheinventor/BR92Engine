import os, sys

CHUNKSIZE = 8

def writeSection(binary, data, msx, msy, x, y, z, sx=0, sz=0, ex=-1, ez=-1):
	shouldWrite = False
	for row in data[sz:ez]:
		for col in row[sx:ex]:
			if col != 2:
				shouldWrite = True
	if not shouldWrite:
		return
	binary.extend(list(x.to_bytes(4, 'little', signed=True)))
	binary.extend(list(y.to_bytes(4, 'little', signed=True)))
	binary.extend(list(z.to_bytes(4, 'little', signed=True)))
	binary.append(msx)
	binary.append(msy)
	for row in data[sz:ez]:
		for col in row[sx:ex]:
			tile = col - 2
			binary.extend(tile.to_bytes(2, 'little'))

if __name__=='__main__':
	if len(sys.argv) >= 4:
		try:
			with open(sys.argv[1]) as f:
				fdata = f.read().split("\n")
		except Exception as e:
			print(f"Error loading level csv \"{sys.argv[1]}\"\nOriginal Error: {str(e)}")
			exit(1)
	else:
		print(f"Usage: {sys.argv[0]} level.csv level.dat x,y,z")
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
	binary.extend([ord(c) for c in "SECT"])
	width = len(data)
	height = len(data[0])
	if width > CHUNKSIZE or height > CHUNKSIZE:
		for zz in range(0, height, CHUNKSIZE):
			for xx in range(0, width, CHUNKSIZE):
				writeSection(binary, data, CHUNKSIZE, CHUNKSIZE, x+xx, y, z+zz, xx, zz, min(xx+CHUNKSIZE, width), min(zz+CHUNKSIZE, height))
	else:
		writeSection(binary, data, width, height, x, y, z)

	if os.path.exists(sys.argv[2]):
		mode = "ab"
	else:
		mode = "wb"
	
	try:
		with open(sys.argv[2], mode) as f:
			f.write(bytes(binary))
	except Exception as e:
		print(f"Error writing output file.\nOriginal Error: {str(e)}")
		exit(1)
