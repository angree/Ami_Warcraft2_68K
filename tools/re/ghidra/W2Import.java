// W2Import.java - Ghidra headless PRE-script for the Warcraft II Mac 68k image built by
// tools/re/mac68k_link.py.  Argument 1: directory holding w2_layout.txt / w2_symbols.txt /
// w2_traps.txt (outside the repository).  Creates the low-memory and A-line trap pseudo blocks,
// sets A5 for the whole image, applies segment / jump-table / function labels.
// Language: 68000:BE:32:MacTraps (tools/re/ghidra/mac68020.slaspec).
//@category W2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.math.BigInteger;
import java.nio.file.*;
import java.util.*;

public class W2Import extends GhidraScript {
	static final Object[][] LOWMEM = {
		{0x102, "ScrVRes"}, {0x104, "ScrHRes"}, {0x108, "MemTop"}, {0x10C, "BufPtr"},
		{0x12D, "LoadTrap"}, {0x130, "ApplLimit"}, {0x16A, "Ticks"}, {0x172, "MBState"},
		{0x174, "KeyMap"}, {0x184, "KeypadMap"}, {0x20C, "Time"}, {0x21E, "KbdType"},
		{0x220, "MemErr"}, {0x28E, "ROM85"}, {0x2A6, "SysZone"}, {0x2AA, "ApplZone"},
		{0x2AE, "ROMBase"}, {0x2F0, "DoubleTime"}, {0x2F4, "CaretTime"}, {0x31A, "Lo3Bytes"},
		{0x824, "ScrnBase"}, {0x828, "MTemp"}, {0x82C, "RawMouse"}, {0x830, "Mouse"},
		{0x8A4, "CrsrRect"}, {0x8CE, "CrsrNew"}, {0x8CF, "CrsrCouple"}, {0x8D0, "CrsrState"},
		{0x8F2, "CrsrBusy"}, {0x904, "CurrentA5"}, {0x908, "CurStackBase"}, {0x910, "CurApName"},
		{0x930, "CurApRefNum"}, {0x934, "CurPageOption"}, {0x936, "LoaderPBlock"},
		{0x9D6, "WindowList"}, {0x9EE, "GrayRgn"}, {0xA02, "OneOne"}, {0xA06, "MinusOne"},
		{0xA1C, "MenuList"}, {0xA50, "TopMapHndl"}, {0xA54, "SysMapHndl"}, {0xA58, "SysMap"},
		{0xA5A, "CurMap"}, {0xA5E, "ResLoad"}, {0xA60, "ResErr"}, {0xA64, "FScaleDisable"},
		{0xAF0, "DeskHook"}, {0xB22, "HiliteMode"}, {0xBB2, "SegHiEnable"}, {0xC20, "SndBase"},
		{0xCC0, "TheGDevice"}, {0x8A4, "CrsrRect"}, {0xD28, "MainDevice"}, {0x8A8, "DeviceList"},
		{0xBAA, "MenuDisable"}, {0x2D4, "SPMisc2"}, {0x9FA, "DragHook"}, {0x8A8, "DeviceList"},
	};

	Map<String, String> layout = new HashMap<>();

	Address a(long v) { return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(v); }

	@Override
	public void run() throws Exception {
		String dir = getScriptArgs().length > 0 ? getScriptArgs()[0] : ".";
		for (String l : Files.readAllLines(Paths.get(dir, "w2_layout.txt"))) {
			String[] p = l.trim().split("\\s+");
			if (p.length == 2) layout.put(p[0], p[1]);
		}
		long base = Long.parseLong(layout.get("IMAGE_BASE"), 16);
		long size = Long.parseLong(layout.get("IMAGE_SIZE"), 16);
		long a5 = Long.parseLong(layout.get("A5"), 16);
		long trapBase = Long.parseLong(layout.get("TRAP_BASE"), 16);
		Memory mem = currentProgram.getMemory();
		SymbolTable st = currentProgram.getSymbolTable();

		// low memory (uninitialised, names only)
		if (mem.getBlock(a(0)) == null) {
			MemoryBlock lm = mem.createUninitializedBlock("lowmem", a(0), 0x3000, false);
			lm.setWrite(true);
		}
		for (Object[] e : LOWMEM) {
			try { st.createLabel(a((Integer) e[0]), (String) e[1], SourceType.IMPORTED); } catch (Exception ex) {}
		}
		// A-line trap pseudo block: every word is "rts"
		if (mem.getBlock(a(trapBase)) == null) {
			MemoryBlock tb = mem.createInitializedBlock("aline_traps", a(trapBase), 0x2000, (byte) 0, monitor, false);
			tb.setExecute(true);
			for (int i = 0; i < 0x2000; i += 2) mem.setShort(a(trapBase + i), (short) 0x4E75);
		}
		for (String l : Files.readAllLines(Paths.get(dir, "w2_traps.txt"))) {
			String[] p = l.trim().split("\\s+");
			Address t = a(Long.parseLong(p[0], 16));
			if (st.getPrimarySymbol(t) == null || st.getPrimarySymbol(t).getSource() == SourceType.DEFAULT)
				st.createLabel(t, p[1], SourceType.IMPORTED);
		}
		// A5 for the whole image
		Register r = currentProgram.getRegister("A5");
		currentProgram.getProgramContext().setValue(r, a(base), a(base + size - 1), BigInteger.valueOf(a5));

		// blocks are one flat image; split per segment would be nicer but is not needed
		List<Address> funcs = new ArrayList<>();
		for (String l : Files.readAllLines(Paths.get(dir, "w2_symbols.txt"))) {
			String[] p = l.trim().split("\\s+");
			if (p.length < 3) continue;
			Address ad = a(Long.parseLong(p[0], 16));
			String kind = p[2];
			if (kind.equals("macsbug")) continue;   // applied by W2Export after analysis
			st.createLabel(ad, p[1], SourceType.IMPORTED);
			if (kind.equals("func") || kind.equals("jt")) funcs.add(ad);
		}
		for (Address ad : funcs) {
			disassemble(ad);
			if (getFunctionAt(ad) == null) createFunction(ad, null);
		}
		// traps that never return
		for (String nm : new String[] {"_ExitToShell", "_SysError", "_Debugger", "_DebugStr"}) {
			for (Symbol s : st.getSymbols(nm)) {
				Function f = getFunctionAt(s.getAddress());
				if (f == null) f = createFunction(s.getAddress(), nm);
				if (f != null) f.setNoReturn(true);
			}
		}
		println("W2Import: A5=" + Long.toHexString(a5) + " funcs=" + funcs.size());
	}
}
