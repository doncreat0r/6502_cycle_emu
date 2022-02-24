#include "Bus.h"

bool Bus::ReadFromFile(const char* filename, uint16_t offset)
{
	std::streampos fileSize;
	std::ifstream file(filename, std::ios::binary);

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	file.read((char*)&RAM[offset], fileSize);

	return true;
}

void Bus::CPU_Step()
{
	if (pins.RW)
		pins.DATA = RAM[pins.ADDR];


	pins = CPU.tick(pins);

	TickHandler();

	if (!pins.RW)
		RAM[pins.ADDR] = pins.DATA;


	if (pins.SYNC)
	{
		opaddr = CPU.readPC();
	}
}

void Bus::CPU_Step_Op()
{
	do
	{
		CPU_Step();
	} while (!pins.SYNC);
}

uint8_t& Bus::operator[](uint16_t addr)
{
	return RAM[addr];
}
