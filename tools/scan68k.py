import capstone, glob, collections, re
md=capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_M68K_040)
traps=collections.Counter(); fpu=0; lowmem=collections.Counter(); funcs=0; ninsn=0
for fn in sorted(glob.glob('re/CODE_*.bin')):
    seg=int(re.findall(r'CODE_(\d+)',fn)[0])
    if seg==0: continue
    d=open(fn,'rb').read()
    funcs+=d.count(b'\x4e\x56')  # link a6
    off=4 if seg==1 else 12
    while off < len(d)-1:
        w=int.from_bytes(d[off:off+2],'big')
        if (w & 0xF000)==0xA000:
            traps[w & 0xF9FF if w & 0x0800 else w & 0xF0FF]+=1; off+=2; ninsn+=1; continue
        ins=None
        for ins in md.disasm(d[off:off+10], off, 1): pass
        if ins is None: off+=2; continue
        ninsn+=1
        if ins.mnemonic.startswith('f'): fpu+=1
        m=re.search(r'\$([0-9a-f]+)\.w', ins.op_str)
        if m:
            a=int(m.group(1),16)
            if a < 0x1000 and '#' not in ins.op_str.split(',')[0][:1]: lowmem[a]+=1
        off+=ins.size
print('insns~',ninsn,'link a6~',funcs,'fpu-ish',fpu,'distinct traps',len(traps),'trap sites',sum(traps.values()))
print(sorted(['%04X:%d'%(k,v) for k,v in traps.items()]))
print('lowmem abs.w', sorted(['%03X:%d'%(k,v) for k,v in lowmem.items()]))
