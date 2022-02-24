#include "debugger.h"

void Debugger::disassemble(uint16_t nStart, uint16_t nStop)
{
	uint32_t addr = nStart;
	uint8_t value = 0x00, lo = 0x00, hi = 0x00;
	uint16_t line_addr = 0;

	mapAsm.clear();
	// A convenient utility to convert variables into
	// hex strings because "modern C++"'s method with 
	// streams is atrocious
	auto hex = [](uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	// Starting at the specified address we read an instruction
	// byte, which in turn yields information from the lookup table
	// as to how many additional bytes we need to read and what the
	// addressing mode is. I need this info to assemble human readable
	// syntax, which is different depending upon the addressing mode

	// As the instruction is decoded, a std::string is assembled
	// with the readable output
	while (addr <= (uint32_t)nStop && addr < 0xFFFD)
	{
		line_addr = addr;

		// Prefix line with instruction address
		std::string sInst = "$" + hex(addr, 4) + ": ";

		// Read instruction, and get its readable name
		uint8_t opcode = bus.RAM[addr]; addr++;
		std::string op = bus.CPU.op_table[opcode].code;
		sInst += op + " ";

		// Get operands from desired locations, and form the
		// instruction based upon its addressing mode. These
		// routines mimmick the actual fetch routine of the
		// 6502 in order to get accurate data as part of the
		// instruction
		if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_imp)
		{
			sInst += " {IMP}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_imm)
		{
			value = bus.RAM[addr]; addr++;
			sInst += "#$" + hex(value, 2) + " {IMM}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_zpg)
		{
			lo = bus.RAM[addr]; addr++;
			hi = 0x00;
			sInst += "$" + hex(lo, 2) + " {ZP0}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_zpg_X)
		{
			lo = bus.RAM[addr]; addr++;
			hi = 0x00;
			sInst += "$" + hex(lo, 2) + ", X {ZPX}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_zpg_Y)
		{
			lo = bus.RAM[addr]; addr++;
			hi = 0x00;
			sInst += "$" + hex(lo, 2) + ", Y {ZPY}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_ind_X)
		{
			lo = bus.RAM[addr]; addr++;
			hi = 0x00;
			sInst += "($" + hex(lo, 2) + ", X) {IZX}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_ind_Y)
		{
			lo = bus.RAM[addr]; addr++;
			hi = 0x00;
			sInst += "($" + hex(lo, 2) + "), Y {IZY}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_abs)
		{
			lo = bus.RAM[addr]; addr++;
			hi = bus.RAM[addr]; addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + " {ABS}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_abs_X)
		{
			lo = bus.RAM[addr]; addr++;
			hi = bus.RAM[addr]; addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + ", X {ABX}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_abs_Y)
		{
			lo = bus.RAM[addr]; addr++;
			hi = bus.RAM[addr]; addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + ", Y {ABY}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_ind)
		{
			lo = bus.RAM[addr]; addr++;
			hi = bus.RAM[addr]; addr++;
			sInst += "($" + hex((uint16_t)(hi << 8) | lo, 4) + ") {IND}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_rel)
		{
			value = bus.RAM[addr]; addr++;
			sInst += "$" + hex(value, 2) + " [$" + hex(addr + value, 4) + "] {REL}";
		}
		else if (bus.CPU.op_table[opcode].addr == &mos6502::Addr_jsr)
		{
			lo = bus.RAM[addr]; addr++;
			hi = bus.RAM[addr]; addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + " {ABS}";
		}

		// Add the formed string to a std::map, using the instruction's
		// address as the key. This makes it convenient to look for later
		// as the instructions are variable in length, so a straight up
		// incremental index is not sufficient.
		mapAsm[line_addr] = sInst;
	}

}
