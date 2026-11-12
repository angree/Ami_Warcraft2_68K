import sys, os, machfs
iso, off, ln, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
with open(iso,'rb') as f:
    f.seek(off*512); data=f.read(ln*512)
v=machfs.Volume(); v.read(data)
print('VOLUME', v.name)
def walk(folder, path):
    for name, item in folder.items():
        p = path + '/' + ''.join(c if c.isprintable() and c not in '\\/:*?"<>|' else '_' for c in name)
        if isinstance(item, machfs.Folder):
            os.makedirs(out+p, exist_ok=True); walk(item, p)
        else:
            print(f"{len(item.data):10d} {len(item.rsrc):9d} {item.type.decode('mac_roman')!r:8} {item.creator.decode('mac_roman')!r:8} {p}")
            with open(out+p,'wb') as o: o.write(item.data)
            if item.rsrc:
                with open(out+p+'.rsrc','wb') as o: o.write(item.rsrc)
os.makedirs(out, exist_ok=True); walk(v, '')
