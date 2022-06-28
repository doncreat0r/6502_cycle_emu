#include <iostream>
#include <sstream>
#include <cstdint>
#include <thread>
#include <atomic>

#include "mos6502.h"
#include "Bus.h"
#include "debugger.h"
#include <conio.h>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine\olcPixelGameEngine.h"

class TestBus : public Bus
{
public:
	virtual void TickHandler() 
	{
		//
	}
};

class EhBasicBus : public Bus
{
public:
	virtual void TickHandler()
	{
		uint16_t a = pins.ADDR;
		if (pins.RW) 
		{
			if (a == 0xF004)
			{
				if (_kbhit())
					pins.DATA = _getch();
				else
					pins.DATA = 0;
				RAM[0xF004] = pins.DATA;
			}
		}
		else
		{
			if (a == 0xF001) 
				std::cout << pins.DATA << std::flush;
		}
	}
};

class Demo6502 : public olc::PixelGameEngine
{
public:
	EhBasicBus nes;
	Debugger deb = {nes};

private:
	bool cpu_init_done;
	bool cpu_done;
	
	std::thread thread_cpu;
	bool step_mode = false;
	bool step_one = false;

	void cpu_task();

	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	void DrawRam(int x, int y, uint16_t nAddr, int nRows, int nColumns)
	{
		int nRamX = x, nRamY = y;
		for (int row = 0; row < nRows; row++)
		{
			std::string sOffset = "$" + hex(nAddr, 4) + ":";
			for (int col = 0; col < nColumns; col++)
			{
				sOffset += " " + hex(nes[nAddr], 2);
				nAddr += 1;
			}
			DrawString(nRamX, nRamY, sOffset);
			nRamY += 10;
		}
	}

	void DrawCpu(int x, int y)
	{
		Flags6502& F = deb.getFlags();
		Registers6502& R = deb.getRegisters();

		std::string status = "STATUS: ";
		DrawString(x , y , "STATUS:", olc::WHITE);
		DrawString(x  + 64, y, "N", F.N ? olc::GREEN : olc::RED);
		DrawString(x  + 80, y , "V", F.V ? olc::GREEN : olc::RED);
		DrawString(x  + 96, y , "-", F.X ? olc::GREEN : olc::RED);
		DrawString(x  + 112, y , "B", F.B ? olc::GREEN : olc::RED);
		DrawString(x  + 128, y , "D", F.D ? olc::GREEN : olc::RED);
		DrawString(x  + 144, y , "I", F.I ? olc::GREEN : olc::RED);
		DrawString(x  + 160, y , "Z", F.Z ? olc::GREEN : olc::RED);
		DrawString(x  + 178, y , "C", F.C ? olc::GREEN : olc::RED);
		DrawString(x , y + 10, "PC: $" + hex(R.PC, 4));
		DrawString(x , y + 20, "A: $" +  hex(R.A, 2) + "  [" + std::to_string(R.A) + "]");
		DrawString(x , y + 30, "X: $" +  hex(R.X, 2) + "  [" + std::to_string(R.X) + "]");
		DrawString(x , y + 40, "Y: $" +  hex(R.Y, 2) + "  [" + std::to_string(R.Y) + "]");
		DrawString(x , y + 50, "Stack P: $" + hex(R.SP, 4));
	}

	void DrawCode(int x, int y, int nLines)
	{
		std::atomic<uint16_t>& PC = nes.opaddr;

		auto it_a = deb.mapAsm.find(PC);
		int nLineY = (nLines >> 1) * 10 + y;
		if (it_a != deb.mapAsm.end())
		{
			DrawString(x, nLineY, (*it_a).second, olc::CYAN);
			while (nLineY < (nLines * 10) + y)
			{
				nLineY += 10;
				if (it_a != deb.mapAsm.end())
				{
					++it_a;
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}

		it_a = deb.mapAsm.find(PC);
		nLineY = (nLines >> 1) * 10 + y;
		if (it_a != deb.mapAsm.end())
		{
			while (nLineY > y)
			{
				nLineY -= 10;
				if (it_a != deb.mapAsm.end() && it_a != deb.mapAsm.begin())
				{
					--it_a;
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}
	}

public:
	Demo6502()
	{
		sAppName = "Demo6502";
	}


	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		thread_cpu = std::thread(&Demo6502::cpu_task, this);

		nes.ReadFromFile("tests//ehbasic.bin", 0xC000);

		deb.disassemble(0x0000, 0xFFFF);

		nes.pins = nes.CPU.reset();
		cpu_init_done = true;
		
		return true;
	}

	bool OnUserDestroy() override
	{
		cpu_done = true;
		thread_cpu.join();
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// called once per frame
		Clear(olc::DARK_BLUE);

		if (GetKey(olc::Key::SPACE).bPressed)
		{
			step_mode = true;
			step_one = true;
		}

		if (GetKey(olc::Key::R).bPressed)
			nes.pins = nes.CPU.reset();

		if (GetKey(olc::Key::I).bPressed)
			nes.pins.IRQ = true;

		if (GetKey(olc::Key::N).bPressed)
			nes.pins.NMI = true;

		if (GetKey(olc::Key::S).bPressed)
			step_mode = false;

		DrawRam(2, 2, 0x0000, 16, 16);
		DrawRam(2, 182, nes.CPU.readPC() & 0xFF00, 16, 16);
		DrawCpu(516, 2);
		DrawCode(448, 72, 26);

		return true;
	}
};

void Demo6502::cpu_task()
{
	for (;;)
	{
		if (!cpu_init_done) continue;
		if (cpu_done) break;

		if (step_mode)
		{
			if (step_one)
			{
				nes.CPU_Step_Op();
				step_one = false;
			}
		}
		else
			nes.CPU_Step();

		//uint16_t pc = nes.CPU.readPC();
		//if (!(nes.CPU.readPC() ^ 0b1111111))
		//	std::this_thread::sleep_for(std::chrono::microseconds(1));
 
		//if (!nes.pins.RES && nes.opaddr == 0x37A3)
		//	nes.pins = nes.CPU.forceJumpTo(0x400);

		if (nes.pins.SYNC)
		{
			nes.pins.IRQ = false;
			nes.pins.NMI = false;
		}
	}
}

static Demo6502 demo;

int main()
{
	if (demo.Construct(780, 480, 2, 2))
		demo.Start();

	std::cout << std::dec << "Total ticks: " << demo.nes.CPU.readTicksTotal() << std::endl;
	return 0;
}
