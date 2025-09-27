# video proxy format

header:
- magic (VPFX)
- version
- width
- height
- frame count
- index offset (where can we find the table with offsets)

after the header, it's the frames. each frame is Zstd compressed with level 3.

after the frames, it's the index table that has the frame index and offset
