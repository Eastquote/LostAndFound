import sys
import os
import math
import re
import io
import numpy as np
from PIL import Image
import hashlib

# Version
atlasFormatVersion = 1

def PalettizeImage(img, paletteImg, numColors):
	# Convert to an indexed image (may remove colors)
	indexedImg = None
	if paletteImg is not None:
		indexedImg = img.convert('RGBA').convert(mode='P', dither='NONE', palette=paletteImg.im)
	else:
		indexedImg = img.convert('RGBA').convert(mode='P', dither='NONE', colors=numColors)

	# Save and load the image to update the info (including transparency)
	f = io.BytesIO()
	indexedImg.save(f, 'png')
	indexedImg = Image.open(f)
	# Reinterpret the indexed image as a grayscale image
	grayscaleImg = Image.fromarray(np.asarray(indexedImg), 'L')
	# Create the palette
	palette = indexedImg.getpalette()
	transparency = indexedImg.info.get("transparency", -1)
	try:
		transparency = list(transparency)
	except TypeError:
		transparentColorIdx = transparency
		transparency = [0xff] * 256
		transparency[transparentColorIdx] = 0
	if paletteImg is None:
		paletteColors = np.asarray([[palette[(3 * i) : (3 * i + 3)] + [transparency[i]] \
			for i in range(numColors)]]).astype('uint8')
		paletteImg = Image.fromarray(paletteColors, mode='RGBA')
	grayscaleRgbImg = Image.new("RGBA", grayscaleImg.size)
	grayscaleRgbImg.paste(grayscaleImg)
	return grayscaleRgbImg, paletteImg

def PackAnims(path):
	atlasName = os.path.basename(path)
	print("Packing '" + atlasName + "'...")

	# Discover pngs + anims
	print("  Discovering anims...")
	metaFn = os.path.join(path, "_meta.txt")
	anims = {}
	def GatherAnims(anims, path, animName):
		for fn in os.listdir(path):
			if "Palette_" in fn:
				continue
			fullFn = os.path.join(path, fn)
			name, suffix = os.path.splitext(fn)
			if(suffix == ".png"):
				if animName is None:
					anims[name] = [ fullFn ]
				else:
					anims[animName].append(fullFn)
			elif os.path.isdir(fullFn):
				anims[name] = []
				GatherAnims(anims, fullFn, name)
	GatherAnims(anims, path, None)

	# Load metadata file
	meta = ""
	try:
		with open(metaFn) as f:
			meta = f.read()
	except:
		pass

	# Read metadata settings
	def GetMeta(tag):
		p = re.compile(r'.*' + tag + ' (\d+).*', re.IGNORECASE | re.DOTALL)
		m = p.match(meta)
		if m is not None:
			return int(m.group(1))
		return None
	metaPadding = GetMeta('padding')
	padding = 1 if metaPadding is None else metaPadding
	metaPowerOfTwo = GetMeta('power_of_two')
	powerOfTwo = 0 if metaPowerOfTwo is None else metaPowerOfTwo
	metaFrameRate = GetMeta('framerate')
	frameRate = 30 if metaFrameRate is None else metaFrameRate
	metaPalettize = GetMeta('palettize')
	palettize = 0 if metaPalettize is None else metaPalettize

	# Load all images (replacing filename with image)
	print("  Loading anims...")
	spriteDims = None
	totalImages = 0
	for anim in anims.values():
		for imageIdx in range(len(anim)):
			fn = anim[imageIdx]
			img = Image.open(fn)
			print(f"Loaded {fn}")
			totalImages += 1
			anim[imageIdx] = img
			if spriteDims is None:
				spriteDims = img.size
			else:
				assert(spriteDims == img.size)
	paddedSpriteDims = (spriteDims[0] + padding * 2, spriteDims[1] + padding * 2)

	# Generate atlas image
	print("  Packing atlas image...")
	gridDims = None
	gridAspect = None
	for i in range(totalImages):
		i += 1
		dims = (i, math.ceil(totalImages / i))
		aspect = (dims[0] * spriteDims[0]) / (dims[1] * spriteDims[1])
		aspect = math.fabs(1 - aspect)
		if gridAspect is None or aspect < gridAspect:
			gridAspect = aspect
			gridDims = dims
	totalGridTiles = gridDims[0] * gridDims[1]
	imageDims = (gridDims[0] * paddedSpriteDims[0], gridDims[1] * paddedSpriteDims[1])
	def PowerOfTwo(x):  
		return 1 if x == 0 else 2**(x - 1).bit_length()
	if powerOfTwo:
		imageDims = (PowerOfTwo(imageDims[0]), PowerOfTwo(imageDims[1]))
	atlasImg = Image.new('RGBA', imageDims)
	animIdx = 0
	for anim in anims.values():
		for imageIdx in range(len(anim)):
			img = anim[imageIdx]
			gridPos = (animIdx % gridDims[0], math.floor(animIdx / gridDims[0]))
			imgPos = (gridPos[0] * paddedSpriteDims[0] + padding, gridPos[1] * paddedSpriteDims[1] + padding)
			atlasImg.paste(img, imgPos)
			anim[imageIdx] = animIdx
			animIdx += 1

	# Palettize atlas (optional)
	paletteImg = None
	paletteImgFn = ""
	if palettize:
		paletteImg = None
		try:
			paletteBaseImgFn = path + "/Palette_Base.png"
			paletteImg = Image.open(paletteBaseImgFn)
		except:
			pass
		paletteImgFn = 'anims/' + atlasName + '_Palette_Base.png'
		atlasImg, paletteImg = PalettizeImage(atlasImg, paletteImg, 256)
		paletteImg.save(paletteBaseImgFn)

	# Generate atlas metadata
	atlasMeta = "# Meta\n"
	atlasMeta += "name " + atlasName + "\n"
	atlasMeta += "num_sprites " + str(totalImages) + "\n"
	atlasMeta += "dims_image " + str(imageDims[0]) + " " + str(imageDims[1]) + "\n"
	atlasMeta += "dims_frame " + str(spriteDims[0]) + " " + str(spriteDims[1]) + "\n"
	atlasMeta += "dims_padded " + str(paddedSpriteDims[0]) + " " + str(paddedSpriteDims[1]) + "\n"
	atlasMeta += "dims_grid " + str(gridDims[0]) + " " + str(gridDims[1]) + "\n"
	if metaPadding is None:
		atlasMeta += "padding 1" + "\n"
	if metaFrameRate is None:
		atlasMeta += "framerate " + str(frameRate) + "\n"
	atlasMeta += meta + "\n"
	atlasMeta += "# Anims\n"
	for name, anim in anims.items():
		atlasMeta += name
		for imageIdx in anim:
			atlasMeta += " " + str(imageIdx)
		atlasMeta += "\n"

	# Hash to check for changes
	print("  Checking hash...")
	if not os.path.isdir('anims'):
		os.mkdir('anims')
	atlasMetaFn = 'anims/' + atlasName + '.txt'
	hashStr = str(atlasFormatVersion) + atlasMeta;
	if paletteImg is not None:
		hashObj = hashlib.md5(hashStr.encode() + atlasImg.tobytes() + paletteImg.tobytes())
	else:
		hashObj = hashlib.md5(hashStr.encode() + atlasImg.tobytes())
	hashDigest = str(hashObj.hexdigest())
	if os.path.isfile(atlasMetaFn):
		with open(atlasMetaFn) as f:
			oldHashDigest = f.readline().strip()
			if oldHashDigest == hashDigest:
				print("*** No files written! (hash match)")
				return
	atlasMeta = hashDigest + "\n\n" + atlasMeta

	# Save atlas image
	print("  Saving atlas image...")
	atlasImgFn = 'anims/' + atlasName + '.png'
	atlasImg.save(atlasImgFn)

	# Save palette image (optional)
	if palettize:
		print("  Saving palette image...")
		paletteImg.save(paletteImgFn)

	# Save atlas metadata
	print("  Saving atlas metadata...")
	with open(atlasMetaFn, 'w') as f:
		f.write(atlasMeta)
	print("*** Atlas files written!")

# Main function
def Main():
	path = "anims_src"
	if len(sys.argv) > 1:
		path = sys.argv[1]
	path = os.path.abspath(path)
	if os.path.basename(path) == "anims_src":
		for fn in os.listdir(path):
			fn = os.path.abspath(os.path.join(path, fn))
			if os.path.isdir(fn):
				PackAnims(fn)
				print("")
	else:
		PackAnims(path)

if __name__ == "__main__":
	Main()
