import sys
# Extract track 1 (MODE1/2352) user data -> .iso (2048 B/sector)
src, dst, nsect = sys.argv[1], sys.argv[2], int(sys.argv[3])
with open(src,'rb') as f, open(dst,'wb') as o:
    for i in range(nsect):
        s = f.read(2352)
        if len(s) < 2352: break
        o.write(s[16:16+2048])
