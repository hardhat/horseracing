#!/usr/bin/python3
# Analyze a GIF file and print out dimensions, palette size, transparency color, and frame count.
import PIL.Image

# Check for parameters: analysegif.py <filename>
import sys

if len(sys.argv) != 2:
    print("Usage: analyzegif.py <filename>")
    sys.exit(1)

filename = sys.argv[1]

try:
    with PIL.Image.open(filename) as img:
        print(f"Dimensions: {img.width}x{img.height}")
        print(f"Palette size: {len(img.getpalette()) // 3} colors")
        transparency = img.info.get('transparency')
        if transparency is not None:
            print(f"Transparency color index: {transparency}")
        else:
            print("No transparency color")
        frame_count = getattr(img, "n_frames", 1)
        print(f"Frame count: {frame_count}")
except Exception as e:
    print(f"Error opening image: {e}")
    sys.exit(1)

