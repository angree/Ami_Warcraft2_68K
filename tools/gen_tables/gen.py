# Generator for src/w2_tables.h / src/w2_tables.c
# Inputs: wargus.json (Lua harness dump of Wargus scripts), udta.json (UDTA/UGRD from Mac PUDs),
#         names.json (unit name strings from Mac "War Strings" entry 1), wargus wartool.h (entry numbers).
import json, re, sys, os
from common import *

OUT_H, OUT_C = sys.argv[1], sys.argv[2]
NAMES = json.load(open('names.json', encoding='utf-8'))
NONE = 0xFF

def cid(s):
    return re.sub(r'[^A-Za-z0-9]', '_', s).upper()

def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

# ------------------------------------------------------------------ wartool.h entries
WT = []   # (type, version, path, arg1, arg2)
txt = open(r'D:/GITHUB/_AmiW2-poza-repo/wargus/wartool.h', encoding='latin1').read()
body = txt[txt.index('static Control Todo[]'):]
joined = re.sub(r'\n(?=\s*\d)', ' ', body)  # join wrapped entries "...path",\n2, 121 _2},
for m in re.finditer(r'^\{([A-Z]),(\d+),"([^"]*)",\s*(\d+)(?:,\s*(\d+))?', joined, re.M):
    t, ver, path, a1, a2 = m.group(1), int(m.group(2)), m.group(3), int(m.group(4)), m.group(5)
    WT.append((t, ver, path, a1, int(a2) if a2 else None))

def expand(path):
    def rep(mm):
        neg, i = mm.group(1), int(mm.group(2))
        s = NAMES[i]
        if neg: s = s[s.index(' ') + 1:]
        return ''.join('_' if ch in ' -' else ch.lower() for ch in s)
    return re.sub(r'%(-?)(\d+)', lambda mm: rep(mm), path)

GFX = {}     # expanded path -> (entry, palette)
SND = {}     # path -> entry (War Sounds / sfxdat numbering)
SND_EXP = set()
for t, ver, path, a1, a2 in WT:
    if t in ('G', 'U') and ver in (0, 3):
        GFX.setdefault(expand(path), (a2, a1))
    if t == 'W' and ver in (4, 6):
        SND.setdefault(path, a1)
        if ver == 6: SND_EXP.add(path)

def gfx_of(pngpath):
    if not pngpath: return 0
    p = pngpath[:-4] if pngpath.endswith('.png') else pngpath
    if p in GFX: return GFX[p][0]
    return 0

ERAS = ['summer', 'winter', 'wasteland', 'swamp']

# ------------------------------------------------------------------ sounds
SOUNDDEF = {}   # name -> ('files', [paths]) | ('group', sel, ann)
for n, f in M['sounds']:
    SOUNDDEF[n] = ('files', f if isinstance(f, list) else [f])
for n, a, b in M['soundgroups']:
    SOUNDDEF[n] = ('group', a, b)
ALIAS = {a: b for a, b in M['mapsounds']}
def resolve_sound(n):
    seen = 0
    while n in ALIAS and n not in SOUNDDEF and seen < 10:
        n = ALIAS[n]; seen += 1
    return n if n in SOUNDDEF else None

SOUNDS = []      # list of names in id order
SOUNDIDX = {}
def sound_id(n):
    if n is None: return NONE
    r = resolve_sound(n)
    if r is None:
        return NONE
    if r in SOUNDIDX: return SOUNDIDX[r]
    d = SOUNDDEF[r]
    if d[0] == 'group':
        sound_id(d[1]); sound_id(d[2])
    SOUNDIDX[r] = len(SOUNDS); SOUNDS.append(r)
    return SOUNDIDX[r]

# ------------------------------------------------------------------ missiles
MISSILE_ORDER = ['missile-lightning', 'missile-griffon-hammer', 'missile-dragon-breath', 'missile-fireball',
    'missile-flame-shield', 'missile-blizzard', 'missile-death-and-decay', 'missile-big-cannon', 'missile-exorcism',
    'missile-heal-effect', 'missile-touch-of-death', 'missile-rune', 'missile-whirlwind', 'missile-catapult-rock',
    'missile-ballista-bolt', 'missile-arrow', 'missile-axe', 'missile-submarine-missile', 'missile-turtle-missile',
    'missile-small-fire', 'missile-big-fire', 'missile-impact', 'missile-normal-spell', 'missile-explosion',
    'missile-small-cannon', 'missile-cannon-explosion', 'missile-cannon-tower-explosion', 'missile-daemon-fire',
    'missile-green-cross', 'missile-none', 'missile-death-coil']
MISSILEIDX = {n: i for i, n in enumerate(MISSILE_ORDER)}
MDEF = {n: t for n, t in M['missiles']}
MCLASS = ['missile-class-none', 'missile-class-point-to-point', 'missile-class-point-to-point-with-hit',
          'missile-class-point-to-point-bounce', 'missile-class-stay', 'missile-class-cycle-once', 'missile-class-fire',
          'missile-class-flame-shield', 'missile-class-land-mine', 'missile-class-whirlwind', 'missile-class-death-coil',
          'missile-class-parabolic']

# ------------------------------------------------------------------ upgrades (UGRD order)
UPG_ORDER = ['sword1', 'sword2', 'battle-axe1', 'battle-axe2', 'arrow1', 'arrow2', 'throwing-axe1', 'throwing-axe2',
    'human-shield1', 'human-shield2', 'orc-shield1', 'orc-shield2', 'human-ship-cannon1', 'human-ship-cannon2',
    'orc-ship-cannon1', 'orc-ship-cannon2', 'human-ship-armor1', 'human-ship-armor2', 'orc-ship-armor1', 'orc-ship-armor2',
    'catapult1', 'catapult2', 'ballista1', 'ballista2', 'ranger', 'longbow', 'ranger-scouting', 'ranger-marksmanship',
    'berserker', 'light-axes', 'berserker-scouting', 'berserker-regeneration', 'ogre-mage', 'paladin',
    'holy-vision', 'healing', 'exorcism', 'flame-shield', 'fireball', 'slow', 'invisibility', 'polymorph', 'blizzard',
    'eye-of-kilrogg', 'bloodlust', 'raise-dead', 'death-coil', 'whirlwind', 'haste', 'unholy-armor', 'runes',
    'death-and-decay']
assert len(UPG_ORDER) == 52
UPGIDX = {'upgrade-' + n: i for i, n in enumerate(UPG_ORDER)}
WUPG = {}
for u in M['upgrades']:
    if u.get('Icon'): WUPG.setdefault(u['Name'], u)
ICONS = {i['Name']: i['Frame'] for i in M['icons']}

# ------------------------------------------------------------------ spells
SPELL_ORDER = ['holy-vision', 'healing', 'exorcism', 'fireball', 'slow', 'flame-shield', 'invisibility', 'polymorph',
    'blizzard', 'eye-of-vision', 'bloodlust', 'runes', 'death-coil', 'haste', 'raise-dead', 'whirlwind',
    'unholy-armor', 'death-and-decay', 'suicide-bomber']
SPELLIDX = {'spell-' + n: i for i, n in enumerate(SPELL_ORDER)}
SPELLIDX['spell-eye-of-vision-double-head'] = SPELLIDX['spell-eye-of-vision']
SPELLIDX['spell-bloodlust-double-head'] = SPELLIDX['spell-bloodlust']
SPELLIDX['spell-runes-double-head'] = SPELLIDX['spell-runes']
SPDEF = {}
for n, a in M['spells']:
    d = {}
    for i in range(0, len(a) - 1):
        if isinstance(a[i], str) and a[i] in ('manacost', 'range', 'target', 'sound-when-cast', 'depend-upgrade', 'action', 'condition'):
            d[a[i]] = a[i + 1]
    if 'repeat-cast' in a: d['repeat'] = True
    SPDEF.setdefault(n, {}).update(d)

# ------------------------------------------------------------------ animations
ANIMS = {}
for n, t in M['anims']:
    n2 = re.sub(r'-size-\d+-sight-\d+$', '', n)
    if n2 not in ANIMS and isinstance(t, dict): ANIMS[n2] = t
ANIM_KINDS = ['Still', 'Move', 'Attack', 'Death', 'Harvest_wood', 'Repair', 'SpellCast', 'Active', 'Train', 'Research', 'Upgrade']

# ------------------------------------------------------------------ unit records
EXPANSION_IDS = {0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x23, 0x2C, 0x2E, 0x2F}
UNUSED_IDS = {0x22, 0x24, 0x25, 0x30, 0x36}
TIDES = lambda i: i not in EXPANSION_IDS and i not in UNUSED_IDS
NAMES[0x68 + 2] = NAMES[0x68 + 2] or 'Wall'
EQUIV = {}
for a, b in [("unit-town-hall", "unit-great-hall"), ("unit-keep", "unit-stronghold"), ("unit-castle", "unit-fortress"),
    ("unit-peasant", "unit-peon"), ("unit-elven-lumber-mill", "unit-troll-lumber-mill"), ("unit-human-blacksmith", "unit-orc-blacksmith"),
    ("unit-inventor", "unit-alchemist"), ("unit-stables", "unit-ogre-mound"), ("unit-church", "unit-altar-of-storms"),
    ("unit-mage-tower", "unit-temple-of-the-damned"), ("unit-gryphon-aviary", "unit-dragon-roost"), ("unit-human-barracks", "unit-orc-barracks"),
    ("unit-farm", "unit-pig-farm"), ("unit-footman", "unit-grunt"), ("unit-archer", "unit-axethrower"), ("unit-ranger", "unit-berserker"),
    ("unit-knight", "unit-ogre"), ("unit-paladin", "unit-ogre-mage"), ("unit-mage", "unit-death-knight"), ("unit-dwarves", "unit-goblin-sappers"),
    ("unit-ballista", "unit-catapult"), ("unit-balloon", "unit-zeppelin"), ("unit-gryphon-rider", "unit-dragon"),
    ("unit-human-watch-tower", "unit-orc-watch-tower"), ("unit-human-guard-tower", "unit-orc-guard-tower"),
    ("unit-human-cannon-tower", "unit-orc-cannon-tower"), ("unit-human-shipyard", "unit-orc-shipyard"), ("unit-human-refinery", "unit-orc-refinery"),
    ("unit-human-foundry", "unit-orc-foundry"), ("unit-human-oil-platform", "unit-orc-oil-platform"), ("unit-human-oil-tanker", "unit-orc-oil-tanker"),
    ("unit-human-submarine", "unit-orc-submarine"), ("unit-human-destroyer", "unit-orc-destroyer"), ("unit-battleship", "unit-ogre-juggernaught"),
    ("unit-human-transport", "unit-orc-transport"), ("unit-attack-peasant", "unit-attack-peon"), ("unit-human-wall", "unit-orc-wall"),
    ("unit-human-start-location", "unit-orc-start-location")]:
    EQUIV[a] = b; EQUIV[b] = a
UID = {n: i for i, n in enumerate(PUDNAMES) if n}

def enum_unit(i):
    n = PUDNAMES[i]
    return 'W2U_' + (cid(n[5:]) if n else 'UNUSED_%02X' % i)

HUMAN_SET = set()
ORC_SET = set()
for n, t in UNITS.items():
    img = t.get('Image', [])
    f = img[1] if len(img) > 1 and img[0] == 'file' else ''
    files = M['unitfiles'].get(n, {})
    s = f + ' ' + ' '.join(files.values()) if isinstance(files, dict) else f
    if '/human/' in '/' + s or s.startswith('human/'): HUMAN_SET.add(n)
    if '/orc/' in '/' + s or s.startswith('orc/'): ORC_SET.add(n)

def race_of(i, n):
    if n in HUMAN_SET and n not in ORC_SET: return 'W2_RACE_HUMAN'
    if n in ORC_SET and n not in HUMAN_SET: return 'W2_RACE_ORC'
    # heroes/specials (grapics borrowed): derive from sounds
    t = UNITS.get(n, {})
    snd = ' '.join(x for x in t.get('Sounds', []) if isinstance(x, str))
    if 'human' in snd: return 'W2_RACE_HUMAN'
    if 'orc' in snd: return 'W2_RACE_ORC'
    return 'W2_RACE_NEUTRAL'

CORPSES = ['none', 'unit-human-dead-body', 'unit-orc-dead-body', 'unit-dead-sea-body', 'unit-destroyed-1x1-place',
           'unit-destroyed-2x2-place', 'unit-destroyed-3x3-place', 'unit-destroyed-3x3-place-water', 'unit-destroyed-4x4-place']
CONS_NAMES = ['construction-land', 'construction-land2', 'construction-wall', 'construction-human-shipyard',
    'construction-human-oil-well', 'construction-human-refinery', 'construction-human-foundry', 'construction-orc-shipyard',
    'construction-orc-oil-well', 'construction-orc-refinery', 'construction-orc-foundry']

# trainers / builders from buttons
BUILT_BY = {}; UPG_FROM = {}; RESEARCHED_AT = {}
for b in M['buttons']:
    a = b.get('Action'); v = b.get('Value'); fu = b.get('ForUnit', [])
    if a in ('train-unit', 'build') and v in UID:
        for f in fu:
            if f in UID: BUILT_BY.setdefault(v, f)
    if a == 'upgrade-to' and v in UID:
        for f in fu:
            if f in UID: UPG_FROM.setdefault(v, f)
    if a == 'research' and v in UPGIDX:
        for f in fu:
            if f in UID: RESEARCHED_AT.setdefault(v, f)

# animations used
USED_ANIMS = []
def anim_id(name):
    if name not in USED_ANIMS: USED_ANIMS.append(name)
    return USED_ANIMS.index(name)
for c in ['animations-human-dead-body', 'animations-orc-dead-body', 'animations-dead-sea-body',
          'animations-destroyed-place', 'animations-destroyed-place-water']:
    anim_id(c)

RMB = {'none': 0, 'attack': 1, 'move': 2, 'harvest': 3, 'haul-oil': 4, 'demolish': 5, 'sail': 6}

UNITREC = []
NOTES = []
for i, n in enumerate(PUDNAMES):
    t = UNITS.get(n, {})
    c = costs(t) if t else {}
    ud = {k: U[k][i] for k in U if isinstance(U[k], list) and len(U[k]) >= 110}
    ud['mouse2'] = U['mouse2'][i] if i < 58 else 0
    has_udta = (ud['hp'] or ud['flags'] or ud['size'])
    r = {}
    r['enum'] = enum_unit(i)
    r['name'] = NAMES[i + 2].strip() or (t.get('Name', '') if t else '')
    r['ident'] = n[5:] if n else ''
    xf = []
    if i in UNUSED_IDS: xf.append('W2UX_UNUSED')
    if i in EXPANSION_IDS: xf.append('W2UX_EXPANSION')
    src_udta = bool(has_udta) and TIDES(i)
    if not src_udta and t: xf.append('W2UX_STATS_WARGUS')
    sz = (ud['size'] & 0xffff, ud['size'] >> 16); bx = (ud['box'] & 0xffff, ud['box'] >> 16)
    if src_udta:
        r.update(hp=ud['hp'], armor=ud['armor'], bd=ud['basic'], pd=ud['pierce'], maxr=ud['range'], sight=ud['sight'],
                 time=ud['time'], gold=ud['gold'] * 10, wood=ud['wood'] * 10, oil=ud['oil'] * 10, tw=sz[0], th=sz[1],
                 bw=bx[0], bh=bx[1], pts=ud['points'], prio=ud['prio'], rc=ud['reactc'], rh=ud['reacth'], decay=ud['decay'],
                 annoy=ud['annoy'], flags=ud['flags'], tgt=ud['target'], missile=ud['missile'], mouse=ud['mouse2'],
                 magic=ud['magic'], wu=ud['wupg'], au=ud['aupg'], rect=ud['rect'], ov=ud['overlap'], utype=ud['type'])
    else:
        # Wargus-only data (expansion heroes etc.)
        tw, th = t.get('TileSize', [0, 0]) if t else (0, 0)
        bw, bh = t.get('BoxSize', [0, 0]) if t else (0, 0)
        fl = 0
        if t:
            if t.get('LandUnit'): fl |= 1
            if t.get('AirUnit'): fl |= 2
            if t.get('ExplodeWhenKilled'): fl |= 4
            if t.get('SeaUnit'): fl |= 8
            if t.get('Building'): fl |= 0x20
            if t.get('CanAttack'): fl |= 1 << 19
            if t.get('organic'): fl |= 1 << 27
            if t.get('CanCastSpell'): fl |= 1 << 17
            if 'hero' in t and t['hero']: fl |= 1 << 23
            if t.get('isundead'): fl |= 1 << 15
        tg = (1 if t.get('CanTargetLand') else 0) | (2 if t.get('CanTargetSea') else 0) | (4 if t.get('CanTargetAir') else 0) if t else 0
        mis = MISSILEIDX.get(t.get('Missile', 'missile-none'), 29) if t else 0
        r.update(hp=t.get('HitPoints', 0) if t else ud['hp'], armor=t.get('Armor', 0) if t else ud['armor'],
                 bd=t.get('BasicDamage', 0) if t else ud['basic'], pd=t.get('PiercingDamage', 0) if t else ud['pierce'],
                 maxr=t.get('MaxAttackRange', 0) if t else ud['range'], sight=t.get('SightRange', 0) if t else ud['sight'],
                 time=c.get('time', 0) if t else ud['time'], gold=c.get('gold', 0) if t else ud['gold'] * 10,
                 wood=c.get('wood', 0) if t else ud['wood'] * 10, oil=c.get('oil', 0) if t else ud['oil'] * 10,
                 tw=tw if t else sz[0], th=th if t else sz[1], bw=bw if t else bx[0], bh=bh if t else bx[1],
                 pts=t.get('Points', 0) if t else ud['points'], prio=t.get('Priority', 0) if t else ud['prio'],
                 rc=t.get('ComputerReactionRange', 0) if t else ud['reactc'], rh=t.get('PersonReactionRange', 0) if t else ud['reacth'],
                 decay=0 if t else ud['decay'], annoy=t.get('AnnoyComputerFactor', 0) if t else ud['annoy'],
                 flags=fl if t else ud['flags'], tgt=tg if t else ud['target'], missile=mis if t else ud['missile'],
                 mouse=RMB.get(t.get('RightMouseAction', 'none'), 0) if t else ud['mouse2'],
                 magic=1 if (t and 'Mana' in t) else ud['magic'], wu=0, au=0, rect=1 if (t and t.get('SelectableByRectangle')) else ud['rect'],
                 ov=0, utype=(1 if t.get('AirUnit') else 2 if t.get('SeaUnit') else 0) if t else ud['type'])
    fl = r['flags']
    if fl & 0x20: kind = 'W2_KIND_BUILDING'
    elif fl & 2 or r['utype'] == 1: kind = 'W2_KIND_AIR'
    elif fl & 8 or r['utype'] == 2: kind = 'W2_KIND_SEA'
    elif fl & 1: kind = 'W2_KIND_LAND'
    elif n in ('unit-human-start-location', 'unit-orc-start-location'): kind = 'W2_KIND_MARKER'
    elif r['utype'] == 0 and r['hp'] > 0: kind = 'W2_KIND_LAND'   # e.g. ballista/catapult lack the UDTA land bit
    else: kind = 'W2_KIND_NONE'
    if t and t.get('Building') and kind != 'W2_KIND_BUILDING': kind = 'W2_KIND_BUILDING'
    r['kind'] = kind
    if not (r['flags'] & (1 << 19)): r['missile'] = 29   # cannot attack -> no missile (UDTA stores 0 for buildings)
    r['race'] = race_of(i, n) if n else 'W2_RACE_NEUTRAL'
    if n in ('unit-gold-mine', 'unit-oil-patch', 'unit-circle-of-power', 'unit-dark-portal', 'unit-runestone', 'unit-critter', 'unit-daemon', 'unit-skeleton'):
        r['race'] = 'W2_RACE_NEUTRAL' if n not in ('unit-skeleton', 'unit-daemon') else 'W2_RACE_ORC'
    if n == 'unit-human-start-location': r['race'] = 'W2_RACE_HUMAN'
    if n == 'unit-orc-start-location': r['race'] = 'W2_RACE_ORC'
    r['minr'] = t.get('MinAttackRange', 0) if t else 0
    r['speed'] = t.get('Speed', 0) if t else 0
    if r['speed'] == 99: r['speed'] = 0
    food = 0
    if t:
        if t.get('Supply'): food = t['Supply']
        elif t.get('Demand'): food = -t['Demand']
    r['food'] = food
    # extra flags
    if r['wu']: xf.append('W2UX_WEAPON_UPG')
    if r['au']: xf.append('W2UX_ARMOR_UPG')
    if r['magic']: xf.append('W2UX_MANA')
    if r['rect']: xf.append('W2UX_RECT_SELECT')
    if t:
        if t.get('ExplodeWhenKilled'): xf.append('W2UX_EXPLODES')
        if t.get('BuilderOutside'): xf.append('W2UX_BUILDER_OUTSIDE')
        if t.get('Coward'): xf.append('W2UX_COWARD')
        if t.get('MaxOnBoard'): xf.append('W2UX_TRANSPORT')
        if t.get('GroundAttack'): xf.append('W2UX_GROUND_ATTACK')
        if t.get('MainFacility'): xf.append('W2UX_MAIN_HALL')
        if t.get('ShoreBuilding'): xf.append('W2UX_SHORE')
        if t.get('CanHarvest') or t.get('GivesResource'): xf.append('W2UX_RESOURCE')
        if t.get('Indestructible'): xf.append('W2UX_INDESTRUCTIBLE')
        if t.get('Teleporter'): xf.append('W2UX_TELEPORTER')
        if t.get('DetectCloak'): xf.append('W2UX_DETECT_INVIS')
        if t.get('RepairRange'): xf.append('W2UX_CAN_REPAIR')
        if t.get('Vanishes') or t.get('RandomMovementProbability'): pass
    store = t.get('CanStore', []) if t else []
    st = 0
    for s in store:
        st |= {'gold': 1, 'wood': 2, 'oil': 4}.get(s, 0)
    r['store'] = st
    r['xflags'] = xf
    imp = t.get('ImproveProduction') if t else None
    r['imp_res'] = {'gold': 1, 'wood': 2, 'oil': 3}.get(imp[0], 0) if imp else 0
    r['imp_pct'] = imp[1] if imp else 0
    r['transport'] = t.get('MaxOnBoard', 0) if t else 0
    # links
    r['corpse'] = CORPSES.index(t['Corpse']) if t and t.get('Corpse') in CORPSES else 0
    r['cons'] = CONS_NAMES.index(t['Construction']) if t and t.get('Construction') in CONS_NAMES else NONE
    r['anim'] = anim_id(t['Animations']) if t and t.get('Animations') in ANIMS and n not in ('',) else NONE
    r['icon'] = ICONS.get(t.get('Icon'), NONE) if t else NONE
    r['built_by'] = UID.get(BUILT_BY.get(n), NONE)
    r['upg_from'] = UID.get(UPG_FROM.get(n), NONE)
    r['equiv'] = UID.get(EQUIV.get(n), NONE)
    snd = t.get('Sounds', []) if t else []
    sd = {snd[k]: snd[k + 1] for k in range(0, len(snd) - 1, 2)}
    r['snd'] = [sound_id(sd.get(k)) for k in ('selected', 'acknowledge', 'ready', 'help', 'dead')]
    # graphics
    gf = []; fw = fh = 0
    img = t.get('Image', []) if t else []
    imgd = {img[k]: img[k + 1] for k in range(0, len(img) - 1, 2)} if img else {}
    files = M['unitfiles'].get(n, {}) if n else {}
    for e in ERAS:
        p = files.get(e) if isinstance(files, dict) and files else imgd.get('file')
        gf.append(gfx_of(p) if p else 0)
    if 'size' in imgd: fw, fh = imgd['size']
    r['gfx'] = gf; r['fw'] = fw; r['fh'] = fh
    r['wargus_diff'] = {}
    if t and src_udta:
        wd = dict(hp=t.get('HitPoints'), armor=t.get('Armor', 0), bd=t.get('BasicDamage'), pd=t.get('PiercingDamage'),
                  maxr=t.get('MaxAttackRange', 0), sight=t.get('SightRange'), time=c.get('time', 0), gold=c.get('gold', 0),
                  wood=c.get('wood', 0), oil=c.get('oil', 0), tw=t.get('TileSize', [0, 0])[0], prio=t.get('Priority'))
        r['wargus_diff'] = {k: v for k, v in wd.items() if v is not None and r[k] != v}
    UNITREC.append(r)

# harvest info
HARV = []
for i, n in enumerate(PUDNAMES):
    t = UNITS.get(n)
    if not t or not t.get('CanGatherResources'): continue
    if not (UNITREC[i]['flags'] & ((1 << 8) | (1 << 9))): continue   # only real workers/tankers (UDTA flags)
    for g in t['CanGatherResources']:
        d = {}
        k = 0
        while k < len(g):
            if g[k] in ('terrain-harvester', 'refinery-harvester', 'harvest-from-outside'):
                d[g[k]] = True; k += 1
            else:
                d[g[k]] = g[k + 1]; k += 2
        HARV.append(dict(unit=i, res={'gold': 1, 'wood': 2, 'oil': 3}[d['resource-id']], cap=d.get('resource-capacity', 0),
                         step=d.get('resource-step', 0), war=d.get('wait-at-resource', 0), wad=d.get('wait-at-depot', 0),
                         gfx_loaded=gfx_of(d.get('file-when-loaded')), gfx_empty=gfx_of(d.get('file-when-empty')),
                         terrain=1 if d.get('terrain-harvester') else 0))

# ------------------------------------------------------------------ animations -> steps
STEPS = []  # (frame, wait, arg, flags, comment)
ANIMSETS = []
AF = dict(ATTACK=1, SOUND=2, UNBREAK_BEGIN=4, UNBREAK_END=8, IDLE_TURN=16, DIE=32)
def conv_seq(seq):
    out = []
    cur = dict(frame=None, move=0, flags=0, snd=None)
    last_frame = 0
    pending_turn = 0
    for cmd in seq:
        op = cmd.split()
        k = op[0]
        if k == 'frame': cur['frame'] = int(op[1])
        elif k == 'move': cur['move'] += int(op[1])
        elif k == 'attack': cur['flags'] |= AF['ATTACK']
        elif k == 'sound': cur['snd'] = ' '.join(op[1:])
        elif k == 'unbreakable': cur['flags'] |= AF['UNBREAK_BEGIN'] if op[1] == 'begin' else AF['UNBREAK_END']
        elif k == 'random-goto': pending_turn = 1
        elif k == 'random-rotate': pending_turn = int(op[1])
        elif k in ('label', 'spawn-unit', 'wiggle', 'goto', 'if-var', 'rotate'): pass
        elif k == 'wait':
            f = cur['frame'] if cur['frame'] is not None else last_frame
            last_frame = f
            flags = cur['flags']
            arg = cur['move']
            if cur['snd']:
                assert arg == 0
                flags |= AF['SOUND']; arg = sound_id(cur['snd'])
                assert arg != NONE, cur['snd']
            if pending_turn:
                assert arg == 0 and not (flags & AF['SOUND'])
                flags |= AF['IDLE_TURN']; arg = pending_turn; pending_turn = 0
            out.append([f, int(op[1]), arg, flags])
            cur = dict(frame=None, move=0, flags=0, snd=None)
        else:
            raise Exception('unknown anim op ' + cmd)
    if cur['flags'] or cur['frame'] is not None:
        out.append([cur['frame'] if cur['frame'] is not None else last_frame, 0, cur['move'], cur['flags']])
    return out

for an in USED_ANIMS:
    spec = ANIMS[an]
    seqs = {}
    for kname, seq in spec.items():
        if not isinstance(seq, list): continue
        if kname == 'Death' and [x for x in seq if not x.startswith('spawn-unit')] == ['wait 1']: continue  # Wargus revealer stub
        if any(s.startswith('if-var v.ResourceActive') for s in seq):
            gi = seq.index('goto start'); li = seq.index('label active')
            seqs['Still'] = conv_seq(seq[2:gi]); seqs['Active'] = conv_seq(seq[li + 1:])
        else:
            seqs[kname] = conv_seq(seq)
    ent = {}
    for kname in ANIM_KINDS:
        if kname in seqs:
            s = seqs[kname]
            if kname == 'Death' and s:
                s[-1][3] |= AF['DIE']
            ent[kname] = (len(STEPS), len(s))
            STEPS.extend(s)
    ANIMSETS.append((an, ent))

# ------------------------------------------------------------------ constructions
CONSREC = []
for cn in CONS_NAMES:
    gf = []; size = None; stages = None
    for e in ERAS:
        d = dict(W['cons_' + e])[cn]
        gf.append(gfx_of(d['Files']['File']))
        size = d['Files']['Size']; stages = d['Constructions']
    CONSREC.append((cn, gf, size, stages))

# ------------------------------------------------------------------ buttons
BACT = ['move', 'stop', 'attack', 'patrol', 'stand-ground', 'attack-ground', 'harvest', 'return-goods', 'repair',
        'build', 'train-unit', 'upgrade-to', 'research', 'cast-spell', 'unload', 'button', 'explore', 'cancel',
        'cancel-upgrade', 'cancel-train-unit', 'cancel-build', 'demolish']
BREQ = [None, 'check-upgrade', 'check-units-or', 'check-no-research', 'check-upgrade-to', 'check-single-research',
        'check-network', 'check-true', 'check-debug']
BUTTONS = []
for b in M['buttons']:
    act = b['Action']; val = b.get('Value')
    if act == 'cast-spell' and val == 'spell-suicide-bomber': act = 'demolish'
    req = b.get('Allowed'); args = b.get('AllowArg') or []
    for fu in b.get('ForUnit', []):
        if fu in ('*', 'cancel-upgrade', 'cancel-build'): uid = NONE
        elif fu in UID: uid = UID[fu]
        else: continue
        arg = 0
        if act in ('build', 'train-unit', 'upgrade-to'): arg = UID[val]
        elif act == 'research': arg = UPGIDX[val]
        elif act == 'cast-spell': arg = SPELLIDX[val]
        elif act == 'button': arg = int(val)
        ra = [NONE, NONE]
        for k, a in enumerate(args[:2]):
            ra[k] = UPGIDX.get(a, UID.get(a, NONE))
        extra = 0
        if act == 'explore' or (act == 'train-unit' and val == 'unit-attack-peasant'): extra = 1
        if req == 'check-debug': extra = 1
        BUTTONS.append(dict(unit=uid, level=b.get('Level', 0), pos=b.get('Pos', 0), icon=ICONS.get(b['Icon'], NONE),
                            key=b.get('Key', ''), act=act, arg=arg, req=BREQ.index(req), ra=ra, extra=extra))
BUTTONS.sort(key=lambda x: (x['unit'], x['level'], x['pos']))  # stable: keeps variant order

# ------------------------------------------------------------------ dependencies
DEPS = []
for tgt, alts in M['deps']:
    groups = [x for x in alts if x != 'or']   # DefineDependency(t, {a,b}, "or", {c}): varargs are OR groups
    tk, tid = ('W2_DEP_UNIT', UID[tgt]) if tgt in UID else ('W2_DEP_UPGRADE', UPGIDX[tgt])
    for gi, g in enumerate(groups):
        for x in g:
            rk, rid = ('W2_DEP_UNIT', UID[x]) if x in UID else ('W2_DEP_UPGRADE', UPGIDX[x])
            DEPS.append((tk, tid, gi, rk, rid, tgt, x))

# ------------------------------------------------------------------ modifiers
MODS = []
for un, mlist in M['modifiers']:
    if un not in UPGIDX: continue
    stats = []; targets = []; conv = None
    for m in mlist:
        k = m[0]
        if k == 'apply-to': targets.append(m[1])
        elif k == 'convert-to': conv = m[1]
        elif k in ('PiercingDamage', 'Armor', 'SightRange', 'AttackRange'): stats.append((k, m[1]))
        elif k == 'regeneration-rate': stats.append(('Regen', m[1]))
    for tg in targets:
        if tg not in UID: continue
        for k, v in stats:
            MODS.append((UPGIDX[un], UID[tg], {'PiercingDamage': 'W2_STAT_PIERCING', 'Armor': 'W2_STAT_ARMOR',
                        'SightRange': 'W2_STAT_SIGHT', 'AttackRange': 'W2_STAT_RANGE', 'Regen': 'W2_STAT_REGEN'}[k], v))
        if conv:
            MODS.append((UPGIDX[un], UID[tg], 'W2_STAT_CONVERT', UID[conv]))
        if not stats and not conv:
            MODS.append((UPGIDX[un], UID[tg], 'W2_STAT_ENABLE_SPELL', 0))

# ------------------------------------------------------------------ write header
H = []
w = H.append
w('''/*
 * w2_tables.h - Warcraft II game-rules data tables (facts about the game).
 *
 * GENERATED by tools-side script from: Mac "War Data" entries 0/1 (default UDTA/UGRD = authoritative
 * unit and upgrade stats, cross-checked with UDTA/UGRD sections of 8 Mac PUDs),
 * Wargus scripts (GPLv2, used as documentation of game facts: animations, buttons, missiles,
 * spells, sounds), wartool.h archive entry numbers and the Mac "War Strings" unit-name table.
 * Do not edit numbers by hand without updating docs/SPEC_RULES.md.
 *
 * Conventions:
 *  - gnu99, no floats, all tables const, no endian-dependent layout (plain C structs).
 *  - Unit type id == PUD unit id (0x00..0x68).
 *  - W2_NONE (0xFF) = "no value" for any 8-bit index field; 0 for archive entry fields.
 *  - Archive entry numbers ("gfx", "entry") refer to the Mac "War Data" file (== PC maindat.war
 *    numbering) and sounds to "War Sounds" (== PC sfxdat.sud numbering), as in wartool.h.
 *  - Times: build/train/research "time" fields are W2 time units; see W2_TIME_UNIT_CYCLES.
 *  - Animation waits are game cycles; see W2_CYCLES_PER_SECOND_NORMAL.
 *  - Direction convention: 8 headings, 0=N,1=NE,2=E,3=SE,4=S,5=SW,6=W,7=NW. Sprites store 5
 *    columns (N,NE,E,SE,S); SW/W/NW are drawn by mirroring SE/E/NE horizontally. A unit
 *    animation "frame" value F is the base row offset; the sprite frame is F + column, see
 *    w2_sprite_frame(). Buildings/neutral objects use absolute frames (1 direction).
 */
#ifndef W2_TABLES_H
#define W2_TABLES_H

#include <stdint.h>

#define W2_NONE                 0xFFu
#define W2_NUM_UNIT_TYPES       0x69u   /* PUD ids 0x00..0x68 */
#define W2_UDTA_SLOTS           110u    /* UDTA arrays have 110 slots */
#define W2_NUM_UPGRADES         52u     /* UGRD order */
#define W2_NUM_ERAS             4u

/* ---- timing ----------------------------------------------------------- */
#define W2_CYCLES_PER_SECOND_NORMAL 30u  /* game cycles per second at "normal" speed (Stratagus/Wargus) */
#define W2_TIME_UNIT_CYCLES     6u      /* 1 build/train/research time unit = 6 cycles (NIEPEWNE, see spec) */
#define W2_MANA_MAX             255u
#define W2_MANA_START           84u     /* initial mana of a new caster (Wargus) */
#define W2_GOLD_PER_PUD_UNIT    2500u   /* PUD UNIT.data for gold mine/oil patch * 2500 = amount */
#define W2_TILE_PX              32u

enum w2_era { W2_ERA_FOREST = 0, W2_ERA_WINTER = 1, W2_ERA_WASTELAND = 2, W2_ERA_SWAMP = 3 };
enum w2_race { W2_RACE_HUMAN = 0, W2_RACE_ORC = 1, W2_RACE_NEUTRAL = 2 };
enum w2_kind { W2_KIND_NONE = 0, W2_KIND_LAND, W2_KIND_AIR, W2_KIND_SEA, W2_KIND_BUILDING, W2_KIND_MARKER };
enum w2_resource { W2_RES_NONE = 0, W2_RES_GOLD = 1, W2_RES_WOOD = 2, W2_RES_OIL = 3 };
/* can_target bits (UDTA "can target") */
#define W2_TGT_LAND 1u
#define W2_TGT_SEA  2u
#define W2_TGT_AIR  4u
/* store bits */
#define W2_STORE_GOLD 1u
#define W2_STORE_WOOD 2u
#define W2_STORE_OIL  4u
/* right mouse button default action (UDTA "2nd mouse button action") */
enum w2_rmb { W2_RMB_NONE = 0, W2_RMB_ATTACK, W2_RMB_MOVE, W2_RMB_HARVEST, W2_RMB_HAUL_OIL, W2_RMB_DEMOLISH, W2_RMB_SAIL };

/* ---- UDTA flags word (bit-exact copy of the PUD UDTA "flags" ULONG) ----- */
#define W2UF_LAND           (1ul << 0)
#define W2UF_AIR            (1ul << 1)
#define W2UF_EXPLODE_DEATH  (1ul << 2)
#define W2UF_SEA            (1ul << 3)
#define W2UF_CRITTER        (1ul << 4)   /* set on critter, skeleton, daemon (NIEPEWNE meaning) */
#define W2UF_BUILDING       (1ul << 5)
#define W2UF_SUBMARINE      (1ul << 6)
#define W2UF_SEE_SUBMARINE  (1ul << 7)
#define W2UF_WORKER         (1ul << 8)
#define W2UF_TANKER         (1ul << 9)
#define W2UF_TRANSPORT      (1ul << 10)
#define W2UF_OIL_PLATFORM   (1ul << 11)
#define W2UF_STORE_HALL     (1ul << 12)   /* town hall tiers: gold + wood depot */
#define W2UF_GROUND_ATTACK  (1ul << 14)
#define W2UF_UNDEAD         (1ul << 15)
#define W2UF_SHORE_BUILDING (1ul << 16)
#define W2UF_SPELLCASTER    (1ul << 17)
#define W2UF_STORE_WOOD     (1ul << 18)
#define W2UF_CAN_ATTACK     (1ul << 19)
#define W2UF_TOWER          (1ul << 20)
#define W2UF_OIL_PATCH      (1ul << 21)
#define W2UF_GOLD_MINE      (1ul << 22)
#define W2UF_HERO           (1ul << 23)
#define W2UF_STORE_OIL      (1ul << 24)
#define W2UF_VOLATILE       (1ul << 25)  /* demolition squad / sappers (kamikaze) */
#define W2UF_MAGE           (1ul << 26)  /* mage / death knight class casters (NIEPEWNE meaning) */
#define W2UF_ORGANIC        (1ul << 27)

/* ---- our own extra flags ---------------------------------------------- */
#define W2UX_UNUSED          (1u << 0)   /* empty PUD slot */
#define W2UX_EXPANSION       (1u << 1)   /* not part of Tides of Darkness (Mac 1.2x): BDP-only unit */
#define W2UX_STATS_WARGUS    (1u << 2)   /* UDTA had no data; numbers come from Wargus */
#define W2UX_WEAPON_UPG      (1u << 3)   /* UDTA weapons upgradable */
#define W2UX_ARMOR_UPG       (1u << 4)   /* UDTA armor upgradable */
#define W2UX_MANA            (1u << 5)
#define W2UX_RECT_SELECT     (1u << 6)   /* selectable by drag rectangle */
#define W2UX_EXPLODES        (1u << 7)   /* spawns explosion missile when killed */
#define W2UX_BUILDER_OUTSIDE (1u << 8)   /* worker stays outside while building */
#define W2UX_COWARD          (1u << 9)   /* does not auto-attack */
#define W2UX_TRANSPORT       (1u << 10)
#define W2UX_GROUND_ATTACK   (1u << 11)
#define W2UX_MAIN_HALL       (1u << 12)
#define W2UX_SHORE           (1u << 13)
#define W2UX_RESOURCE        (1u << 14)  /* gold mine / oil patch */
#define W2UX_INDESTRUCTIBLE  (1u << 15)
#define W2UX_TELEPORTER      (1u << 16)
#define W2UX_DETECT_INVIS    (1u << 17)
#define W2UX_CAN_REPAIR      (1u << 18)
''')

w('/* ---- unit type ids (== PUD unit ids) ---------------------------------- */')
w('enum w2_unit_id {')
for i, r in enumerate(UNITREC):
    w('    %-32s = 0x%02X,%s' % (r['enum'], i, ('   /* %s */' % r['name']) if r['name'] else ''))
w('};')
w('')
w('enum w2_corpse { ' + ', '.join('W2_CORPSE_' + cid(c.replace('unit-', '')) for c in CORPSES) + ' };')
w('')
w('enum w2_missile_id {   /* == UDTA missile byte; also War Data entry 324 + id for 0..28 */')
for i, n in enumerate(MISSILE_ORDER):
    w('    W2M_%s = %d,' % (cid(n[8:]), i))
w('    W2_NUM_MISSILES = %d' % len(MISSILE_ORDER))
w('};')
w('')
w('enum w2_missile_class { ' + ', '.join('W2MC_' + cid(c[14:]) for c in MCLASS) + ' };')
w('')
w('enum w2_upgrade_id {   /* == UGRD index */')
for i, n in enumerate(UPG_ORDER):
    w('    W2UP_%s = %d,' % (cid(n), i))
w('};')
w('')
w('enum w2_spell_id {')
for i, n in enumerate(SPELL_ORDER):
    w('    W2SP_%s = %d,' % (cid(n), i))
w('    W2_NUM_SPELLS = %d' % len(SPELL_ORDER))
w('};')
w('')
w('enum w2_construction_id {')
for i, n in enumerate(CONS_NAMES):
    w('    W2C_%s = %d,' % (cid(n[13:]), i))
w('    W2_NUM_CONSTRUCTIONS = %d' % len(CONS_NAMES))
w('};')
w('')
w('enum w2_anim_set_id {')
for i, (an, ent) in enumerate(ANIMSETS):
    w('    W2A_%s = %d,' % (cid(an[11:]), i))
w('    W2_NUM_ANIM_SETS = %d' % len(ANIMSETS))
w('};')
w('')
w('''enum w2_anim_kind {
    W2_ANIM_STILL = 0,   /* idle; loops */
    W2_ANIM_MOVE,        /* one full tile step (sum of move px == 32 for land units); loops per tile */
    W2_ANIM_ATTACK,      /* one attack cycle; W2AF_ATTACK step fires the hit/missile */
    W2_ANIM_DEATH,       /* plays once; last step has W2AF_DIE -> replace by corpse */
    W2_ANIM_HARVEST,     /* worker chopping wood */
    W2_ANIM_REPAIR,      /* worker repairing / building */
    W2_ANIM_SPELLCAST,
    W2_ANIM_ACTIVE,      /* gold mine with miner inside / oil platform being drilled */
    W2_ANIM_TRAIN,       /* building training a unit */
    W2_ANIM_RESEARCH,
    W2_ANIM_UPGRADE,     /* building being upgraded (shows frame 1) */
    W2_NUM_ANIM_KINDS
};

/* animation step flags */
#define W2AF_ATTACK         0x01u  /* deal damage / launch missile at this step */
#define W2AF_SOUND          0x02u  /* arg = sound id to play */
#define W2AF_UNBREAK_BEGIN  0x04u  /* from here the sequence must not be interrupted by new orders */
#define W2AF_UNBREAK_END    0x08u
#define W2AF_IDLE_TURN      0x10u  /* before waiting: with 1% chance turn by arg heading steps (random dir) */
#define W2AF_DIE            0x20u  /* end of death animation */

typedef struct w2_anim_step {
    uint8_t frame;   /* base frame (add direction column for 5-dir sprites) */
    uint8_t wait;    /* cycles to stay on this step (0 = instant) */
    uint8_t arg;     /* move: pixels to advance this step; with W2AF_SOUND: sound id; with W2AF_IDLE_TURN: turn amount */
    uint8_t flags;   /* W2AF_* */
} w2_anim_step;

typedef struct w2_anim_set {
    const char *name;
    uint16_t first[W2_NUM_ANIM_KINDS];  /* index into w2_anim_steps[], 0xFFFF = none */
    uint8_t  count[W2_NUM_ANIM_KINDS];
} w2_anim_set;
''')

w('''typedef struct w2_unit_type {
    const char *name;           /* display name (Mac "War Strings" unit names) */
    const char *ident;          /* short identifier (Wargus ident without "unit-") */
    uint32_t flags;             /* W2UF_* (UDTA flags word) */
    uint32_t xflags;            /* W2UX_* */
    uint16_t hp;
    uint16_t gold, wood, oil;   /* cost in full units (UDTA stores /10) */
    uint16_t points;            /* score for killing */
    uint16_t gfx[W2_NUM_ERAS];  /* War Data entry of the sprite per era (0 = none) */
    uint8_t  race;              /* enum w2_race */
    uint8_t  kind;              /* enum w2_kind */
    uint8_t  tile_w, tile_h;    /* footprint in tiles (UDTA "size") */
    uint8_t  box_w, box_h;      /* selection box in pixels (UDTA "box size") */
    uint8_t  frame_w, frame_h;  /* sprite frame size in pixels */
    uint8_t  armor;
    uint8_t  basic_damage;      /* reduced by armor */
    uint8_t  piercing_damage;   /* ignores armor */
    uint8_t  min_range, max_range;   /* tiles; max_range 1 = melee */
    uint8_t  sight;             /* tiles */
    uint8_t  speed;             /* informational (Wargus "Speed"); real movement = MOVE animation */
    uint8_t  build_time;        /* W2 time units (* W2_TIME_UNIT_CYCLES = cycles) */
    int8_t   food;              /* >0: food supplied, <0: food used */
    uint8_t  can_target;        /* W2_TGT_* */
    uint8_t  rmb_action;        /* enum w2_rmb */
    uint8_t  react_cpu, react_human;  /* auto-acquire range (tiles) for computer / human owner */
    uint8_t  priority;          /* target selection priority */
    uint8_t  annoy;             /* AI "annoy computer factor" */
    uint8_t  decay;             /* UDTA decay rate */
    uint8_t  overlap;           /* UDTA "overlap frames" (buildings; NIEPEWNE meaning) */
    uint8_t  missile;           /* enum w2_missile_id (W2M_NONE for melee) */
    uint8_t  corpse;            /* enum w2_corpse */
    uint8_t  anim;              /* enum w2_anim_set_id or W2_NONE */
    uint8_t  construction;      /* enum w2_construction_id or W2_NONE */
    uint8_t  icon;              /* frame in the era icon sheet, W2_NONE = none */
    uint8_t  built_by;          /* unit id that trains/builds it, W2_NONE */
    uint8_t  upgrade_from;      /* building it is upgraded from (keep <- town hall), W2_NONE */
    uint8_t  equiv;             /* other race's equivalent unit, W2_NONE */
    uint8_t  store;             /* W2_STORE_* resources accepted (depots) */
    uint8_t  improve_res;       /* enum w2_resource boosted by owning this building */
    uint8_t  improve_pct;       /* +percent for improve_res */
    uint8_t  transport_cap;     /* units carried */
    uint8_t  snd_selected, snd_ack, snd_ready, snd_help, snd_dead;   /* w2_sound ids or W2_NONE */
} w2_unit_type;

typedef struct w2_harvest {
    uint8_t  unit;              /* worker / tanker unit id */
    uint8_t  resource;          /* enum w2_resource */
    uint8_t  capacity;          /* amount carried per trip */
    uint8_t  step;              /* amount gathered per wait_at_resource period (0 = all at once) */
    uint16_t wait_at_resource;  /* cycles */
    uint16_t wait_at_depot;     /* cycles */
    uint16_t gfx_loaded;        /* War Data entry of the "carrying" sprite (0 = same) */
    uint16_t gfx_empty;         /* tanker empty sprite (0 = normal sprite) */
    uint8_t  terrain;           /* 1 = harvests terrain (trees) */
    uint8_t  pad;
} w2_harvest;

typedef struct w2_missile {
    const char *name;
    uint16_t gfx;               /* War Data entry, 0 = invisible */
    uint8_t  w, h;              /* frame size px */
    uint8_t  frames;            /* total frames in sprite */
    uint8_t  dirs;              /* 1 or 9 (9 = 5 stored + mirroring, Stratagus convention) */
    uint8_t  cls;               /* enum w2_missile_class */
    uint8_t  speed;             /* pixels per step */
    uint8_t  sleep;             /* cycles between steps */
    uint8_t  splash_range;      /* tiles ("Range" in Wargus) */
    uint8_t  splash_factor;     /* damage divisor for splash (0 = none) */
    uint8_t  bounces;
    uint8_t  impact_missile;    /* W2_NONE */
    uint8_t  impact_sound;      /* W2_NONE */
    uint8_t  draw_level;
} w2_missile;

typedef struct w2_upgrade {
    const char *name;
    uint16_t gold, wood, oil;
    uint8_t  time;              /* W2 time units */
    uint8_t  icon;              /* icon frame (== UGRD icon) */
    uint8_t  race;
    uint8_t  researched_at;     /* building unit id, W2_NONE for free spells */
    uint8_t  group;             /* UGRD group */
    uint8_t  spell;             /* spell enabled, W2_NONE */
    uint32_t ugrd_flags;        /* UGRD flags word */
} w2_upgrade;

enum w2_stat { W2_STAT_PIERCING = 0, W2_STAT_ARMOR, W2_STAT_SIGHT, W2_STAT_RANGE, W2_STAT_REGEN,
               W2_STAT_CONVERT, W2_STAT_ENABLE_SPELL };
typedef struct w2_modifier {
    uint8_t upgrade;            /* enum w2_upgrade_id */
    uint8_t unit;               /* affected unit id */
    uint8_t stat;               /* enum w2_stat */
    int8_t  value;              /* delta; for CONVERT: target unit id (as uint8) */
} w2_modifier;

enum w2_dep_kind { W2_DEP_UNIT = 0, W2_DEP_UPGRADE = 1 };
typedef struct w2_dependency {
    uint8_t kind, id;           /* the thing that has requirements */
    uint8_t alt;                /* OR-group: requirements with equal alt are ANDed, groups are ORed */
    uint8_t req_kind, req_id;   /* required unit (must own a completed one) or researched upgrade */
} w2_dependency;

enum w2_spell_target { W2_ST_SELF = 0, W2_ST_UNIT, W2_ST_POSITION };
enum w2_spell_effect {
    W2_SE_REVEAL = 0,    /* holy vision: summon invisible revealer for ttl cycles */
    W2_SE_HEAL,          /* +p1 HP per cast step, repeat while mana */
    W2_SE_DAMAGE_UNDEAD, /* exorcism: p1 damage per step to undead */
    W2_SE_MISSILE,       /* launch missile with p1 damage */
    W2_SE_BUFF,          /* set timer variable (haste/slow/bloodlust/invisible/unholy) to p2 cycles */
    W2_SE_FLAME_SHIELD,  /* 5 orbiting flame missiles, ttl p2, p1 damage per hit */
    W2_SE_POLYMORPH,     /* turn organic target into neutral critter */
    W2_SE_AREA,          /* blizzard / death and decay: area bombardment, p1 = shards, repeat */
    W2_SE_EYE,           /* summon eye of kilrogg for p2 cycles */
    W2_SE_RUNES,         /* 5 land-mines (+ shape), p1 damage, ttl p2 */
    W2_SE_RAISE_DEAD,    /* summon skeleton from corpse, ttl p2 */
    W2_SE_WHIRLWIND,     /* whirlwind missile ttl p2, p1 damage per tick */
    W2_SE_DEMOLISH       /* explode self: p1 damage in radius p2 tiles */
};
#define W2SF_ORGANIC_ONLY 1u
#define W2SF_UNDEAD_ONLY  2u
#define W2SF_NO_BUILDING  4u
#define W2SF_REPEAT       8u
#define W2SF_NO_AIR       16u
#define W2SF_HALVE_HP     32u   /* unholy armor: costs target half its HP */
typedef struct w2_spell {
    const char *name;
    uint8_t  mana;
    uint8_t  range;             /* tiles, W2_NONE = unlimited */
    uint8_t  target;            /* enum w2_spell_target */
    uint8_t  effect;            /* enum w2_spell_effect */
    uint8_t  upgrade;           /* required research, W2_NONE */
    uint8_t  missile;           /* W2_NONE */
    uint8_t  sound;             /* W2_NONE */
    uint8_t  flags;             /* W2SF_* */
    int16_t  p1;
    uint16_t p2;
} w2_spell;

typedef struct w2_construction {
    const char *name;
    uint16_t gfx[W2_NUM_ERAS];  /* War Data entry of the construction-site sprite per era */
    uint8_t  w, h;              /* frame size px */
    uint8_t  nstages;
    struct { uint8_t percent, use_building_sprite, frame; } stage[3];
    /* stage i applies from 'percent' % of build progress; use_building_sprite=1 means draw
       'frame' of the building's own sprite, else 'frame' of the construction-site sprite.
       At 100 % the building switches to frame 0 of its own sprite. */
} w2_construction;

enum w2_button_action {
''')
for a in BACT:
    w('    W2B_%s,' % cid(a))
w('};')
w('''enum w2_button_req {
    W2R_NONE = 0,          /* always shown */
    W2R_UPGRADE,           /* req_arg[0] upgrade researched (icon variants: later entry wins) */
    W2R_UNITS_OR,          /* player owns any of req_arg[0..1] (unit ids) */
    W2R_NO_RESEARCH,       /* building not currently researching/upgrading */
    W2R_UPGRADE_TO,        /* building idle and target dependency satisfied */
    W2R_SINGLE_RESEARCH,   /* upgrade not yet researched and not in progress anywhere */
    W2R_NETWORK,           /* multiplayer/skirmish only (Wargus: walls) */
    W2R_TRUE,
    W2R_DEBUG              /* Wargus debug-only option */
};
#define W2BF_WARGUS_EXTRA 1u   /* button not in original game (Wargus addition) */
typedef struct w2_button {
    uint8_t unit;      /* unit id, W2_NONE = generic (all units) */
    uint8_t level;     /* 0 = main card, 1/2 = build submenus (basic/advanced), 9 = generic cancel */
    uint8_t pos;       /* 1..9, row-major 3x3 grid (1=top-left) */
    uint8_t icon;      /* icon frame */
    char    key;       /* hotkey (lower case), 27 = ESC */
    uint8_t action;    /* enum w2_button_action */
    uint8_t arg;       /* unit id / upgrade id / spell id / submenu level */
    uint8_t req;       /* enum w2_button_req; dependencies (w2_dependencies) also apply to train/build */
    uint8_t req_arg[2];
    uint8_t bflags;    /* W2BF_* */
} w2_button;

typedef struct w2_sound {
    const char *name;
    uint16_t first;    /* index into w2_sound_entries[] */
    uint8_t  count;    /* number of variants (pick randomly / cycle) */
    uint8_t  annoyed;  /* for "selected" groups: sound id played after many clicks, W2_NONE */
} w2_sound;

typedef struct w2_era_info {
    const char *name;
    uint16_t palette;          /* War Data entry: palette */
    uint16_t tileset[4];       /* War Data entries: palette/megatile/minitile/map (wartool "T") */
    uint16_t icons;            /* War Data entry: icon sheet */
} w2_era_info;

typedef struct w2_game_speed {
    const char *name;
    uint8_t cycles_per_second;
} w2_game_speed;

/* ---- tables ------------------------------------------------------------ */
extern const w2_unit_type   w2_unit_types[W2_NUM_UNIT_TYPES];
extern const w2_anim_set    w2_anim_sets[W2_NUM_ANIM_SETS];
extern const w2_anim_step   w2_anim_steps[];
extern const uint16_t       w2_num_anim_steps;
extern const w2_missile     w2_missiles[W2_NUM_MISSILES];
extern const w2_upgrade     w2_upgrades[W2_NUM_UPGRADES];
extern const w2_modifier    w2_modifiers[];
extern const uint16_t       w2_num_modifiers;
extern const w2_dependency  w2_dependencies[];
extern const uint16_t       w2_num_dependencies;
extern const w2_spell       w2_spells[W2_NUM_SPELLS];
extern const w2_construction w2_constructions[W2_NUM_CONSTRUCTIONS];
extern const w2_button      w2_buttons[];        /* sorted by unit, level, pos; generic (unit=W2_NONE) last */
extern const uint16_t       w2_num_buttons;
extern const w2_harvest     w2_harvest_info[];
extern const uint8_t        w2_num_harvest;
extern const w2_sound       w2_sounds[];
extern const uint8_t        w2_num_sounds;
extern const uint16_t       w2_sound_entries[];  /* War Sounds entry numbers (0xFFFF = missing) */
extern const w2_era_info    w2_eras[W2_NUM_ERAS];
extern const w2_game_speed  w2_game_speeds[];
extern const uint8_t        w2_num_game_speeds;

/* ---- accessors ---------------------------------------------------------- */
const w2_unit_type *w2_unit(unsigned id);                      /* NULL if id out of range */
/* returns first step of animation kind for a unit type (NULL if none); *count = number of steps */
const w2_anim_step *w2_unit_anim(unsigned unit_id, unsigned kind, unsigned *count);
const w2_anim_step *w2_anim(unsigned set, unsigned kind, unsigned *count);
/* buttons of a unit type: returns pointer to first entry and count (contiguous) */
const w2_button *w2_unit_buttons(unsigned unit_id, unsigned *count);
const w2_harvest *w2_unit_harvest(unsigned unit_id, unsigned resource);  /* NULL if cannot */
/* sprite frame for a 5-direction unit: base = anim frame, heading 0..7; *mirror set for 5..7 */
unsigned w2_sprite_frame(unsigned base, unsigned heading, int *mirror);
/* build/train time in game cycles */
unsigned w2_time_to_cycles(unsigned time_units);

#endif /* W2_TABLES_H */
''')
open(OUT_H, 'w', newline='\n').write('\n'.join(H))

# ------------------------------------------------------------------ write C
C = []
w = C.append
w('''/*
 * w2_tables.c - Warcraft II game-rules data tables. GENERATED, see w2_tables.h for provenance.
 * Numbers are game facts (default UDTA/UGRD from Mac War Data entries 0/1, Wargus scripts used as
 * documentation). No code from Wargus/Stratagus is included.
 */
#include <stddef.h>
#include "w2_tables.h"

#define N W2_NONE
''')
w('const w2_unit_type w2_unit_types[W2_NUM_UNIT_TYPES] = {')
for i, r in enumerate(UNITREC):
    xf = ' | '.join(r['xflags']) or '0'
    cm = '/* 0x%02X %s' % (i, r['name'] or '(unused)')
    if r['wargus_diff']:
        cm += ' -- Wargus differs: ' + ', '.join('%s=%s' % (k, v) for k, v in sorted(r['wargus_diff'].items()))
    cm += ' */'
    w('  ' + cm)
    w('  { %s, %s, 0x%08Xul, %s,' % (cstr(r['name']), cstr(r['ident']), r['flags'], xf))
    w('    %d, %d, %d, %d, %d, {%s},' % (r['hp'], r['gold'], r['wood'], r['oil'], r['pts'], ', '.join(str(g) for g in r['gfx'])))
    w('    %s, %s, %d, %d, %d, %d, %d, %d,' % (r['race'], r['kind'], r['tw'], r['th'], r['bw'], r['bh'], r['fw'], r['fh']))
    w('    %d, %d, %d, %d, %d, %d, %d, %d, %d,' % (r['armor'], r['bd'], r['pd'], r['minr'], r['maxr'], r['sight'], r['speed'], r['time'], r['food']))
    w('    %d, %d, %d, %d, %d, %d, %d, %d,' % (r['tgt'], r['mouse'], r['rc'], r['rh'], r['prio'], r['annoy'], r['decay'], r['ov']))
    def nn(v): return 'N' if v == NONE else str(v)
    def nu(v): return 'N' if v == NONE else enum_unit(v)
    w('    %s, %d, %s, %s, %s, %s, %s, %s, %d, %d, %d, %d,' % (
        ('W2M_' + cid(MISSILE_ORDER[r['missile']][8:])) if r['missile'] < len(MISSILE_ORDER) else str(r['missile']),
        r['corpse'], ('W2A_' + cid(USED_ANIMS[r['anim']][11:])) if r['anim'] != NONE else 'N',
        ('W2C_' + cid(CONS_NAMES[r['cons']][13:])) if r['cons'] != NONE else 'N',
        nn(r['icon']), nu(r['built_by']), nu(r['upg_from']), nu(r['equiv']) if r['equiv'] == NONE or r['equiv'] < i else enum_unit(r['equiv']), r['store'], r['imp_res'], r['imp_pct'], r['transport']))
    w('    %s },' % ', '.join(nn(s) for s in r['snd']))
w('};')
w('')

# anim steps
w('const w2_anim_step w2_anim_steps[] = {')
for (an, ent) in ANIMSETS:
    for kname in ANIM_KINDS:
        if kname in ent:
            st, ln = ent[kname]
            w('  /* %d: %s %s */' % (st, an[11:], kname))
            row = ['{%d,%d,%d,0x%02X}' % tuple(s) for s in STEPS[st:st + ln]]
            for k in range(0, len(row), 8):
                w('  ' + ', '.join(row[k:k + 8]) + ',')
w('};')
w('const uint16_t w2_num_anim_steps = %d;' % len(STEPS))
w('')
w('#define X 0xFFFFu')
w('const w2_anim_set w2_anim_sets[W2_NUM_ANIM_SETS] = {')
for (an, ent) in ANIMSETS:
    firsts = []; cnts = []
    for kname in ANIM_KINDS:
        if kname in ent:
            firsts.append(str(ent[kname][0])); cnts.append(str(ent[kname][1]))
        else:
            firsts.append('X'); cnts.append('0')
    w('  { %s, {%s}, {%s} },' % (cstr(an[11:]), ','.join(firsts), ','.join(cnts)))
w('};')
w('#undef X')
w('')

# missiles
w('const w2_missile w2_missiles[W2_NUM_MISSILES] = {')
for i, mn in enumerate(MISSILE_ORDER):
    t = MDEF.get(mn, {})
    gfx = gfx_of(t.get('File')) if t.get('File') else 0
    sz = t.get('Size', [0, 0])
    cls = t.get('Class', 'missile-class-none')
    if cls == 'missile-class-hit': cls = 'missile-class-none'
    im = MISSILEIDX.get(t.get('ImpactMissile'), NONE)
    isnd = sound_id(t.get('ImpactSound')) if t.get('ImpactSound') else NONE
    w('  { %s, %d, %d, %d, %d, %d, W2MC_%s, %d, %d, %d, %d, %d, %s, %s, %d },' % (
        cstr(mn[8:]), gfx, sz[0], sz[1], t.get('Frames', 0), t.get('NumDirections', 1), cid(cls[14:]),
        t.get('Speed', 0), t.get('Sleep', 0), t.get('Range', 0), t.get('SplashFactor', 0), t.get('NumBounces', 0),
        'N' if im == NONE else str(im), 'N' if isnd == NONE else str(isnd), t.get('DrawLevel', 0)))
w('};')
w('')

# upgrades
GU = UD['ugrd']
SPELL_OF_UPG = {}
for sn, sd in SPDEF.items():
    if 'depend-upgrade' in sd and sn in SPELLIDX:
        SPELL_OF_UPG.setdefault(UPGIDX[sd['depend-upgrade']], SPELLIDX[sn])
w('const w2_upgrade w2_upgrades[W2_NUM_UPGRADES] = {')
UPGNOTES = []
for i, un in enumerate(UPG_ORDER):
    wu = WUPG.get('upgrade-' + un, {})
    wc = wu.get('Costs', {})
    gname = NAMES[107 + i].replace(' / ', ' ').strip() if 107 + i < len(NAMES) else un
    race = 'W2_RACE_HUMAN'
    ra = UID.get(RESEARCHED_AT.get('upgrade-' + un), NONE)
    if ra != NONE: race = UNITREC[ra]['race']
    elif un in ('eye-of-kilrogg', 'death-coil'): race = 'W2_RACE_ORC'
    wdiff = []
    for k, idx in (('time', '0'), ('gold', '1'), ('wood', '2'), ('oil', '3')):
        gv = GU[k][i]
        if isinstance(wc, dict) and int(wc.get(idx, 0)) != gv: wdiff.append('%s=%s' % (k, wc.get(idx)))
    sp = SPELL_OF_UPG.get(i, NONE)
    w('  { %s, %d, %d, %d, %d, %d, %s, %s, %d, %s, 0x%08Xul },%s' % (
        cstr(gname), GU['gold'][i], GU['wood'][i], GU['oil'][i], GU['time'][i], GU['icon'][i], race,
        'N' if ra == NONE else '0x%02X' % ra, GU['group'][i], 'N' if sp == NONE else str(sp), GU['flags'][i],
        ('  /* Wargus: ' + ', '.join(wdiff) + ' */') if wdiff else ''))
w('};')
w('')
w('const w2_modifier w2_modifiers[] = {')
for up, un, st, v in MODS:
    w('  { W2UP_%s, %s, %s, %d },' % (cid(UPG_ORDER[up]), UNITREC[un]['enum'], st, v if st != 'W2_STAT_CONVERT' else v))
w('};')
w('const uint16_t w2_num_modifiers = %d;' % len(MODS))
w('')
w('const w2_dependency w2_dependencies[] = {')
for tk, tid, gi, rk, rid, tn, rn in DEPS:
    w('  { %s, %d, %d, %s, %d },  /* %s <- %s */' % (tk, tid, gi, rk, rid, tn, rn))
w('};')
w('const uint16_t w2_num_dependencies = %d;' % len(DEPS))
w('')

# spells (semi-manual: effect params from Wargus definitions)
SP_MANUAL = {
    'holy-vision': ('W2_ST_POSITION', 'W2_SE_REVEAL', 0, 0, 25),
    'healing': ('W2_ST_UNIT', 'W2_SE_HEAL', 'W2SF_ORGANIC_ONLY|W2SF_NO_BUILDING|W2SF_REPEAT', 1, 0),
    'exorcism': ('W2_ST_UNIT', 'W2_SE_DAMAGE_UNDEAD', 'W2SF_UNDEAD_ONLY|W2SF_NO_BUILDING|W2SF_REPEAT', 1, 0),
    'fireball': ('W2_ST_POSITION', 'W2_SE_MISSILE', 0, 20, 0),
    'slow': ('W2_ST_UNIT', 'W2_SE_BUFF', 'W2SF_NO_BUILDING', 0, 1000),
    'flame-shield': ('W2_ST_UNIT', 'W2_SE_FLAME_SHIELD', 'W2SF_NO_BUILDING|W2SF_NO_AIR', 1, 600),
    'invisibility': ('W2_ST_UNIT', 'W2_SE_BUFF', 'W2SF_NO_BUILDING', 0, 2000),
    'polymorph': ('W2_ST_UNIT', 'W2_SE_POLYMORPH', 'W2SF_ORGANIC_ONLY', 0, 0),
    'blizzard': ('W2_ST_POSITION', 'W2_SE_AREA', 'W2SF_REPEAT', 11, 5),
    'eye-of-vision': ('W2_ST_POSITION', 'W2_SE_EYE', 0, 0, 765),
    'bloodlust': ('W2_ST_UNIT', 'W2_SE_BUFF', 'W2SF_ORGANIC_ONLY', 0, 1000),
    'runes': ('W2_ST_POSITION', 'W2_SE_RUNES', 'W2SF_NO_BUILDING', 50, 2000),
    'death-coil': ('W2_ST_POSITION', 'W2_SE_MISSILE', 'W2SF_ORGANIC_ONLY', 50, 0),
    'haste': ('W2_ST_UNIT', 'W2_SE_BUFF', 'W2SF_NO_BUILDING', 0, 1000),
    'raise-dead': ('W2_ST_POSITION', 'W2_SE_RAISE_DEAD', 'W2SF_REPEAT', 0, 3600),
    'whirlwind': ('W2_ST_POSITION', 'W2_SE_WHIRLWIND', 0, 3, 800),
    'unholy-armor': ('W2_ST_UNIT', 'W2_SE_BUFF', 'W2SF_NO_BUILDING|W2SF_HALVE_HP', 0, 500),
    'death-and-decay': ('W2_ST_POSITION', 'W2_SE_AREA', 'W2SF_REPEAT', 11, 5),
    'suicide-bomber': ('W2_ST_SELF', 'W2_SE_DEMOLISH', 0, 400, 3),
}
SP_MISSILE = {'fireball': 'missile-fireball', 'death-coil': 'missile-death-coil', 'blizzard': 'missile-blizzard',
              'death-and-decay': 'missile-death-and-decay', 'runes': 'missile-rune', 'whirlwind': 'missile-whirlwind',
              'flame-shield': 'missile-flame-shield', 'exorcism': 'missile-exorcism', 'healing': 'missile-heal-effect',
              'suicide-bomber': 'missile-explosion'}
w('const w2_spell w2_spells[W2_NUM_SPELLS] = {')
for sn in SPELL_ORDER:
    sd = SPDEF['spell-' + sn]
    tgt, eff, fl, p1, p2 = SP_MANUAL[sn]
    rng = sd.get('range'); rng = NONE if rng == 'infinite' else int(rng)
    up = UPGIDX.get(sd.get('depend-upgrade'), NONE)
    mi = MISSILEIDX.get(SP_MISSILE.get(sn), NONE)
    so = sound_id(sd.get('sound-when-cast')) if sd.get('sound-when-cast') else NONE
    nm = {'eye-of-vision': 'Eye of Kilrogg', 'suicide-bomber': 'Demolish'}.get(sn, sn.replace('-', ' ').title())
    w('  { %s, %d, %s, %s, %s, %s, %s, %s, %s, %d, %d },' % (
        cstr(nm), int(sd.get('manacost', 0)), 'N' if rng == NONE else str(rng), tgt, eff,
        'N' if up == NONE else 'W2UP_' + cid(UPG_ORDER[up]), 'N' if mi == NONE else 'W2M_' + cid(MISSILE_ORDER[mi][8:]),
        'N' if so == NONE else str(so), fl or '0', p1, p2))
w('};')
w('')
w('const w2_construction w2_constructions[W2_NUM_CONSTRUCTIONS] = {')
for cn, gf, size, stages in CONSREC:
    sts = ['{%d, %d, %d}' % (s['Percent'], 1 if s['File'] == 'main' else 0, s['Frame']) for s in stages]
    while len(sts) < 3: sts.append('{0, 0, 0}')
    w('  { %s, {%s}, %d, %d, %d, { %s } },' % (cstr(cn[13:]), ', '.join(str(g) for g in gf), size[0], size[1], len(stages), ', '.join(sts)))
w('};')
w('')
w('const w2_button w2_buttons[] = {')
def rname(b, k):
    v = b['ra'][k]
    if v == NONE: return 'N'
    if b['req'] == 1: return 'W2UP_' + cid(UPG_ORDER[v])
    if b['req'] == 2: return enum_unit(v)
    return str(v)
BUTTONS_SORTED = [b for b in BUTTONS if b['unit'] != NONE] + [b for b in BUTTONS if b['unit'] == NONE]
BINDEX = {}
for k, b in enumerate(BUTTONS_SORTED):
    if b['unit'] != NONE:
        BINDEX.setdefault(b['unit'], [k, 0])[1] += 1
    key = b['key']
    kc = '27' if key == 'ESC' else "'%s'" % key
    arg = b['arg']
    if b['act'] in ('build', 'train-unit', 'upgrade-to'): argc = UNITREC[arg]['enum']
    elif b['act'] == 'research': argc = 'W2UP_' + cid(UPG_ORDER[arg])
    elif b['act'] == 'cast-spell': argc = 'W2SP_' + cid(SPELL_ORDER[arg])
    else: argc = str(arg)
    w('  { %s, %d, %d, %s, %s, W2B_%s, %s, %s, {%s, %s}, %d },' % (
        'N' if b['unit'] == NONE else UNITREC[b['unit']]['enum'], b['level'], b['pos'], 'N' if b['icon'] == NONE else b['icon'],
        kc, cid(b['act']), argc, 'W2R_' + ['NONE', 'UPGRADE', 'UNITS_OR', 'NO_RESEARCH', 'UPGRADE_TO', 'SINGLE_RESEARCH', 'NETWORK', 'TRUE', 'DEBUG'][b['req']],
        rname(b, 0), rname(b, 1), b['extra']))
w('};')
w('const uint16_t w2_num_buttons = %d;' % len(BUTTONS_SORTED))
w('')
w('/* per unit: first button index and count (0xFFFF = none) */')
w('static const uint16_t w2_button_first[W2_NUM_UNIT_TYPES] = {')
row = [str(BINDEX[i][0]) if i in BINDEX else '0xFFFF' for i in range(len(UNITREC))]
for k in range(0, len(row), 12): w('  ' + ', '.join(row[k:k + 12]) + ',')
w('};')
w('static const uint8_t w2_button_count[W2_NUM_UNIT_TYPES] = {')
row = [str(BINDEX[i][1]) if i in BINDEX else '0' for i in range(len(UNITREC))]
for k in range(0, len(row), 16): w('  ' + ', '.join(row[k:k + 16]) + ',')
w('};')
w('')
w('const w2_harvest w2_harvest_info[] = {')
for h in HARV:
    w('  { %s, %d, %d, %d, %d, %d, %d, %d, %d, 0 },' % (UNITREC[h['unit']]['enum'], h['res'], h['cap'], h['step'], h['war'], h['wad'], h['gfx_loaded'], h['gfx_empty'], h['terrain']))
w('};')
w('const uint8_t w2_num_harvest = %d;' % len(HARV))
w('')
# sounds: make sure game sounds referenced too
for gs in ['placement error', 'placement success', 'building construction', 'peasant voice work complete',
           'basic orc voices work complete', 'basic human voices research complete', 'basic orc voices research complete',
           'explosion', 'building destroyed', 'ship sinking', 'tree-chopping', 'burning', 'click']:
    sound_id(gs)
ENTRIES = []
SNDROWS = []
MISSING = []
for sname in SOUNDS:
    d = SOUNDDEF[sname]
    if d[0] == 'files':
        first = len(ENTRIES)
        for f in d[1]:
            p = f[:-4] if f.endswith('.wav') else f
            e = SND.get(p)
            if e is None: MISSING.append(p); e = 0xFFFF
            ENTRIES.append(e)
        SNDROWS.append((sname, first, len(d[1]), NONE))
    else:
        sel = sound_id(d[1]); ann = sound_id(d[2])
        if sel == NONE: SNDROWS.append((sname, 0, 0, ann)); MISSING.append('group:' + sname); continue
        s0 = SNDROWS[sel]
        SNDROWS.append((sname, s0[1], s0[2], ann))
w('const uint16_t w2_sound_entries[] = {')
for k in range(0, len(ENTRIES), 16): w('  ' + ', '.join('0x%04X' % e if e == 0xFFFF else str(e) for e in ENTRIES[k:k + 16]) + ',')
w('};')
w('const w2_sound w2_sounds[] = {')
for k, (sname, first, cnt, ann) in enumerate(SNDROWS):
    w('  { %s, %d, %d, %s },  /* %d */' % (cstr(sname), first, cnt, 'N' if ann == NONE else str(ann), k))
w('};')
w('const uint8_t w2_num_sounds = %d;' % len(SNDROWS))
w('')
w('''const w2_era_info w2_eras[W2_NUM_ERAS] = {
  { "forest",    2,  {2, 3, 4, 5},     356 },
  { "winter",    18, {18, 19, 20, 21}, 357 },
  { "wasteland", 10, {10, 11, 12, 13}, 358 },
  { "swamp",     10, {10, 11, 12, 13}, 358 },  /* expansion only; Mac 1.2x has no swamp data -> wasteland */
};

/* NIEPEWNE: original menu has a speed slider; only "normal" = 30 c/s is confirmed (Wargus default).
   Wargus allows 15..75 in steps of 5. */
const w2_game_speed w2_game_speeds[] = {
  { "slowest", 15 }, { "slow", 22 }, { "normal", 30 }, { "fast", 45 }, { "fastest", 60 },
};
const uint8_t w2_num_game_speeds = 5;

const w2_unit_type *w2_unit(unsigned id)
{
    return id < W2_NUM_UNIT_TYPES ? &w2_unit_types[id] : NULL;
}

const w2_anim_step *w2_anim(unsigned set, unsigned kind, unsigned *count)
{
    const w2_anim_set *s;
    if (set >= W2_NUM_ANIM_SETS || kind >= W2_NUM_ANIM_KINDS) { if (count) *count = 0; return NULL; }
    s = &w2_anim_sets[set];
    if (s->first[kind] == 0xFFFFu) { if (count) *count = 0; return NULL; }
    if (count) *count = s->count[kind];
    return &w2_anim_steps[s->first[kind]];
}

const w2_anim_step *w2_unit_anim(unsigned unit_id, unsigned kind, unsigned *count)
{
    const w2_unit_type *u = w2_unit(unit_id);
    if (!u || u->anim == W2_NONE) { if (count) *count = 0; return NULL; }
    return w2_anim(u->anim, kind, count);
}

const w2_button *w2_unit_buttons(unsigned unit_id, unsigned *count)
{
    if (unit_id >= W2_NUM_UNIT_TYPES || w2_button_first[unit_id] == 0xFFFFu) { if (count) *count = 0; return NULL; }
    if (count) *count = w2_button_count[unit_id];
    return &w2_buttons[w2_button_first[unit_id]];
}

const w2_harvest *w2_unit_harvest(unsigned unit_id, unsigned resource)
{
    unsigned i;
    for (i = 0; i < w2_num_harvest; ++i)
        if (w2_harvest_info[i].unit == unit_id && w2_harvest_info[i].resource == resource)
            return &w2_harvest_info[i];
    return NULL;
}

unsigned w2_sprite_frame(unsigned base, unsigned heading, int *mirror)
{
    heading &= 7u;
    if (heading <= 4u) { if (mirror) *mirror = 0; return base + heading; }
    if (mirror) *mirror = 1;
    return base + (8u - heading);   /* 5=SW->SE(3), 6=W->E(2), 7=NW->NE(1) */
}

unsigned w2_time_to_cycles(unsigned time_units)
{
    return time_units * W2_TIME_UNIT_CYCLES;
}
''')
open(OUT_C, 'w', newline='\n').write('\n'.join(C))

# report
print('units', len(UNITREC), 'animsets', len(ANIMSETS), 'steps', len(STEPS), 'buttons', len(BUTTONS_SORTED),
      'sounds', len(SNDROWS), 'entries', len(ENTRIES), 'missing snd', MISSING, 'mods', len(MODS), 'deps', len(DEPS), 'harv', len(HARV))
json.dump({'units': UNITREC, 'harv': HARV, 'cons': CONSREC}, open('report.json', 'w'), indent=1, default=str)

# ---- sanity report: move animation pixel sums and cycle counts
for an, ent in ANIMSETS:
    if 'Move' in ent:
        st, ln = ent['Move']
        seq = STEPS[st:st + ln]
        px = sum(s[2] for s in seq if not (s[3] & (AF['SOUND'] | AF['IDLE_TURN'])))
        cyc = sum(s[1] for s in seq)
        print('MOVE %-22s px=%3d cycles=%3d' % (an[11:], px, cyc))
    if 'Attack' in ent:
        st, ln = ent['Attack']
        seq = STEPS[st:st + ln]
        print('ATK  %-22s cycles=%3d' % (an[11:], sum(s[1] for s in seq)))
