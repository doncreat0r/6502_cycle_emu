#pragma once
#include <iostream>
#include <fstream>
#include <cstdint>
#include <array>
#include <functional>
#include "mos6502.h"

typedef std::array<uint8_t, 64 * 1024> AllMemory;

class Bus
{
	friend class Debugger;
protected:
	AllMemory RAM = { 0 };

public:
	Pins pins = {};
	mos6502 CPU;
	std::atomic<uint16_t> opaddr;

public:
	Bus() { }
	~Bus() {}

	virtual void TickHandler() = 0;

	bool ReadFromFile(const char* filename, uint16_t offset);

	void AddTickHandler(std::function<void(void)> callback)
	{

	}

	void CPU_Step();
	void CPU_Step_Op();

	// access bus address space (now just RAM) via bus[]
	uint8_t& operator[](uint16_t addr);
};