-- harness: record Define* calls of wargus scripts (read-only use as documentation)
local ROOT = WROOT
local REC = {units={}, anims={}, missiles={}, buttons={}, modifiers={}, allows={}, deps={},
  constructions={}, spells={}, upgrades={}, icons={}, sounds={}, soundgroups={}, mapsounds={},
  unitfiles={}, errors={}, gamesounds={}, ai={}}
_G.REC = REC
local STUB
local smt = {}
smt.__index = function() return STUB end
smt.__newindex = function() end
smt.__call = function() return STUB end
smt.__concat = function(a,b) return "" end
smt.__lt = function() return false end
smt.__le = function() return false end
smt.__add = function() return 0 end
smt.__sub = function() return 0 end
smt.__mul = function() return 0 end
smt.__div = function() return 0 end
smt.__unm = function() return 0 end
smt.__len = function() return 0 end
STUB = setmetatable({}, smt)
_G.STUB = STUB
table.getn = function(t) return #t end
_ = function(s) return s end
wc2 = {preferences = {EnhancedEffects=false, UseFancyShadows=false, UseFancyBuildings=false}}
wargus = {extensions=false, expansion=false, tileset=TILESET, music_extension=".ogg"}
InitFuncs = {add=function(self,f) end}
function Load(p)
  local f, err = loadfile(ROOT.."/"..p)
  if not f then table.insert(REC.errors, "load "..p..": "..tostring(err)); return end
  local ok, e = pcall(f)
  if not ok then table.insert(REC.errors, "run "..p..": "..tostring(e)) end
end
function Rand(n) return {"rand", n} end
function DefineUnitType(n,t) table.insert(REC.units, {n,t}) end
function DefineAnimations(n,t) table.insert(REC.anims, {n,t}) end
function DefineMissileType(n,t) table.insert(REC.missiles, {n,t}) end
function DefineButton(t) table.insert(REC.buttons, t) end
function DefineModifier(n,...) table.insert(REC.modifiers, {n,{...}}) end
function DefineAllow(n,f) table.insert(REC.allows, {n,f}) end
function DefineDependency(n,...) table.insert(REC.deps, {n,{...}}) end
function DefineConstruction(n,t) table.insert(REC.constructions, {n,t}) end
function DefineSpell(n,...) table.insert(REC.spells, {n,{...}}) end
function DefineIcon(t) table.insert(REC.icons, t) end
function MakeSound(n,f) table.insert(REC.sounds, {n,f}); return n end
function MakeSoundGroup(n,a,b) table.insert(REC.soundgroups, {n,a,b}); return n end
function MapSound(a,b) table.insert(REC.mapsounds, {a,b}) end
function DefineGameSounds(...) table.insert(REC.gamesounds, {...}) end
CUpgrade = {New=function(self, n) local u = {Name=n, Costs={}}; table.insert(REC.upgrades, u); return u end}
Icons = setmetatable({}, {__index=function(t,k) return k end})
CIcon = {New=function(self,n) local i={Name=n}; table.insert(REC.icons, i); return i end}
function UnitTypeByIdent(n) return n end
setmetatable(_G, {__index=function(t,k) return STUB end})
