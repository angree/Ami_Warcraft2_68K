import sys, json, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'pylib'))
from lupa import LuaRuntime
WROOT = r'D:/GITHUB/_AmiW2-poza-repo/wargus'
def tojson(L, v, depth=0):
    t = L.eval('type')(v)
    if t == 'table':
        keys = list(v.keys())
        n = len(keys)
        if n and all(isinstance(k, int) for k in keys) and sorted(keys) == list(range(1, n+1)):
            return [tojson(L, v[i], depth+1) for i in range(1, n+1)]
        if n == 0: return []
        return {str(k): tojson(L, v[k], depth+1) for k in keys}
    if t == 'function': return '<function>'
    if t in ('number','string','boolean','nil'): return v
    return '<%s>' % t
def run(tileset, files):
    L = LuaRuntime(unpack_returned_tuples=True)
    L.globals().WROOT = WROOT
    L.globals().TILESET = tileset
    L.execute(open(os.path.join(os.path.dirname(__file__), 'harness.lua')).read())
    for f in files:
        L.globals().Load(f)
    L.execute('REC.unitfiles = rawget(_G, "UnitTypeFiles") or {}')
    return tojson(L, L.globals().REC)
if __name__ == '__main__':
    out = {}
    out['main'] = run('summer', ['scripts/sound.lua', 'scripts/icons.lua', 'scripts/missiles.lua', 'scripts/constructions.lua',
                                 'scripts/units.lua', 'scripts/upgrade.lua', 'scripts/buttons.lua', 'scripts/spells.lua'])
    for ts in ['summer', 'winter', 'wasteland', 'swamp']:
        out['cons_'+ts] = run(ts, ['scripts/constructions.lua'])['constructions']
    json.dump(out, open(sys.argv[1], 'w'), indent=1)
    print('errors', out['main']['errors'])
    for k, v in out['main'].items(): print(k, len(v))
