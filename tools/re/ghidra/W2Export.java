// W2Export.java - Ghidra headless POST-script for the Warcraft II Mac 68k image.
// Arguments: <dir with w2_symbols.txt [w2_names.txt]> <output .c file>
//  1. names the functions that carry MacsBug symbols (CODE 21 = RAD Smacker library),
//  2. applies hand-made names from w2_names.txt ("ADDR name [comment...]") if present,
//  3. decompiles every function into one file, each preceded by "// ==== <addr> <name> [seg+off]"
//     and the strings / traps it references.
// Output is a READING AID for the specification work and must stay OUTSIDE the repository.
//@category W2
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class W2Export extends GhidraScript {
	Address a(long v) { return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(v); }

	void rename(Address ad, String name, SourceType src) {
		Function f = getFunctionContaining(ad);
		if (f == null) { disassemble(ad); f = createFunction(ad, null); }
		if (f == null) return;
		try { f.setName(name, src); } catch (Exception ex) { println("rename failed " + ad + " " + name + ": " + ex); }
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		String dir = args.length > 0 ? args[0] : ".";
		String out = args.length > 1 ? args[1] : "w2_all.c";
		for (String l : Files.readAllLines(Paths.get(dir, "w2_symbols.txt"))) {
			String[] p = l.trim().split("\\s+");
			if (p.length >= 3 && p[2].equals("macsbug")) rename(a(Long.parseLong(p[0], 16)), p[1], SourceType.IMPORTED);
		}
		Path names = Paths.get(dir, "w2_names.txt");
		if (Files.exists(names)) {
			for (String l : Files.readAllLines(names)) {
				l = l.trim();
				if (l.isEmpty() || l.startsWith("#")) continue;
				String[] p = l.split("\\s+", 3);
				Address ad = a(Long.parseLong(p[0], 16));
				Function f = getFunctionAt(ad);
				if (f == null) { disassemble(ad); f = createFunction(ad, null); }
				if (f != null) {
					try { f.setName(p[1], SourceType.USER_DEFINED); } catch (Exception ex) { println("name " + l + ": " + ex); }
					if (p.length > 2) f.setComment(p[2]);
				} else {
					try { currentProgram.getSymbolTable().createLabel(ad, p[1], SourceType.USER_DEFINED); } catch (Exception ex) {}
				}
			}
		}
		// jsr/jmp d16(A5) go through the jump table (loaded entries are "jmp abs.l"): give each
		// call site a call-override reference to the final function so the decompiler shows
		// direct calls instead of (*_JT_mXXXX)().
		long a5 = currentProgram.getProgramContext().getRegisterValue(currentProgram.getRegister("A5"),
			a(0x00110000L)).getUnsignedValue().longValue();
		ReferenceManager rmo = currentProgram.getReferenceManager();
		ghidra.program.model.mem.Memory mem = currentProgram.getMemory();
		int nov = 0;
		InstructionIterator ii = currentProgram.getListing().getInstructions(true);
		while (ii.hasNext()) {
			Instruction ins = ii.next();
			int w0 = mem.getShort(ins.getAddress()) & 0xffff;
			if (w0 != 0x4EAD && w0 != 0x4EED) continue;
			long e = a5 + mem.getShort(ins.getAddress().add(2));
			Address ea = a(e);
			if (!mem.contains(ea) || (mem.getShort(ea) & 0xffff) != 0x4EF9) continue;
			Address tgt = a(mem.getInt(ea.add(2)) & 0xffffffffL);
			if (getFunctionAt(tgt) == null) { disassemble(tgt); createFunction(tgt, null); }
			RefType rt = w0 == 0x4EAD ? RefType.CALL_OVERRIDE_UNCONDITIONAL : RefType.JUMP_OVERRIDE_UNCONDITIONAL;
			for (Reference r : rmo.getReferencesFrom(ins.getAddress()))
				if (r.getReferenceType().isOverride()) rmo.delete(r);
			Reference r = rmo.addMemoryReference(ins.getAddress(), tgt, rt, SourceType.USER_DEFINED, 0);
			rmo.setPrimary(r, true);
			nov++;
		}
		println("W2Export: " + nov + " jump-table call overrides");
		DecompInterface d = new DecompInterface();
		d.openProgram(currentProgram);
		PrintWriter w = new PrintWriter(new BufferedWriter(new OutputStreamWriter(new FileOutputStream(out), "UTF-8")));
		w.println("// Warcraft II Mac 1.23 (68k) - Ghidra decompilation, image built by tools/re/mac68k_link.py");
		w.println("// A5 = 0x00210000; CODE n at 0x00100000 + n*0x8000; A-line traps = calls to 0x00F00000+(trap&0xfff)*2");
		w.println("// READING AID ONLY - never copy into the repository.");
		ReferenceManager rm = currentProgram.getReferenceManager();
		Listing listing = currentProgram.getListing();
		FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
		int n = 0, fail = 0;
		while (it.hasNext() && !monitor.isCancelled()) {
			Function f = it.next();
			long ep = f.getEntryPoint().getOffset();
			if (ep >= 0x00F00000L) continue;     // trap stubs
			if (ep >= 0x00200000L && ep < 0x00220000L) continue;   // jump-table thunks in the A5 world
			String seg = "";
			if (ep >= 0x00100000L && ep < 0x00200000L) seg = String.format(" [CODE %d +0x%X]", (ep - 0x00100000L) / 0x8000, (ep - 0x00100000L) % 0x8000);
			w.println("// ==== " + f.getEntryPoint() + " " + f.getName() + seg);
			if (f.getComment() != null) w.println("//   note: " + f.getComment());
			Set<String> strs = new TreeSet<>();
			for (Address ad : f.getBody().getAddresses(true)) {
				for (Reference r : rm.getReferencesFrom(ad)) {
					Data dt = listing.getDataAt(r.getToAddress());
					if (dt != null && dt.hasStringValue()) {
						String s = String.valueOf(dt.getValue());
						if (s.length() > 120) s = s.substring(0, 120);
						strs.add(s.replace("\n", "\\n").replace("\r", "\\r"));
					}
				}
			}
			for (String s : strs) w.println("//   str: \"" + s + "\"");
			DecompileResults r = d.decompileFunction(f, 90, monitor);
			if (r != null && r.decompileCompleted()) w.println(r.getDecompiledFunction().getC());
			else { w.println("// (decompile failed: " + (r == null ? "null" : r.getErrorMessage().replace('\n', ' ')) + ")"); fail++; }
			n++;
		}
		w.close();
		println("W2Export: " + n + " functions, " + fail + " failed -> " + out);
	}
}
