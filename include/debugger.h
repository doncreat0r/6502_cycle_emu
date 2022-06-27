#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <array>
#include "Bus.h"
#include "mos6502.h"

class Debugger 
{
private:
public:
	Bus& bus;
	std::map<uint16_t, std::string> mapAsm;
public:
	Debugger(Bus& _bus) : bus(_bus) { }
	~Debugger() {}

	Registers6502 getRegisters() { return bus.CPU.R; }
	uint8_t getFlags() { return bus.CPU.S; }
	uint8_t Read(uint16_t addr) { return bus.RAM[addr]; }
	void Write(uint16_t addr, uint8_t data) { bus.RAM[addr] = data; }

	void disassemble(uint16_t nStart, uint16_t nStop);
	size_t getRAMSize() { return bus.RAM.size(); }
};