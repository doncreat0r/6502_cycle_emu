#include <gtest/gtest.h>
#include <mos6502.h>
#include "6502_test_code.h"
#include <Bus.h>
#include <thread>
#include <atomic>
#include <iostream>

class TestBus : public Bus
{
public:
	virtual void TickHandler() 
	{
		//
	}

    // assign memory contents from some const buffer
    void AssignCode(const AllMemory code)
    {
        RAM = code;
    }
};

class TestCPU
{
private:
    TestBus bus;

	bool cpu_init_done = false;
	bool cpu_done = false;
	
	std::unique_ptr<std::thread> thread_cpu;
	bool step_mode = false;
	bool step_one = false;

    std::atomic<uint16_t> loopAddress; // adress at where the CPU is in final loop (either error or passed)
    uint64_t cycles = 0; // total number of cycles consumed

	void cpu_task();

public:
    const uint16_t ADDRESS_TEST_PASSED = 0x3469;

    TestCPU() : loopAddress(0)
    {
		thread_cpu = std::make_unique<std::thread>(&TestCPU::cpu_task, this);

        bus.AssignCode(code);
        // replace starting address in reset vector with a hardcoded one
        bus[0xFFFC] = 0x00;
        bus[0xFFFD] = 0x04;

		bus.pins = bus.CPU.reset();
		cpu_init_done = true;
    }

    uint64_t isTestPassedAtAddress() 
    {
        // thread will stop after passing some (many) cycles or at ADDRESS_TEST_PASSED
        thread_cpu->join();
        return (loopAddress);
    }

    void StopTest() 
    {
        cpu_done = true;
    }

};

// Test RESET CPU state with address zero
TEST(CPUResetZeroTest, BasicAssertions) {
    mos6502 cpu;
    Pins pins = cpu.reset();

    EXPECT_TRUE(pins.RES);
    EXPECT_TRUE(pins.RW);
    EXPECT_TRUE(pins.SYNC);
    EXPECT_FALSE(pins.RDY);
    EXPECT_EQ(cpu.readSP(), 0xFD);
    EXPECT_EQ(cpu.readPC(), 0);
}

// Test CPU with a full 6502 test suite code running in separate thread
TEST(CPUSuiteCodeTestInThread, BasicAssertions) {
    TestCPU cpu;

    EXPECT_EQ(cpu.isTestPassedAtAddress(), cpu.ADDRESS_TEST_PASSED);

}

void TestCPU::cpu_task() 
{
	for (;;)
	{
		if (!cpu_init_done) continue;
		if (cpu_done) break;

		bus.CPU_Step();

        cycles++;
        loopAddress = bus.CPU.readPC();
        
        if (cycles > 100000000 || loopAddress == ADDRESS_TEST_PASSED) break;

		if (bus.pins.SYNC)
		{
			bus.pins.IRQ = false;
			bus.pins.NMI = false;
		}
	}
}