#include <gtest/gtest.h>
#include <mos6502.h>

// Test RESET CPU state
TEST(CPUResetTest, BasicAssertions) {
    mos6502 cpu;
    Pins pins = cpu.reset();

    EXPECT_TRUE(pins.RES);
    EXPECT_TRUE(pins.RW);
    EXPECT_TRUE(pins.SYNC);
    EXPECT_FALSE(pins.RDY);
    EXPECT_EQ(cpu.readSP(), 0xFD);
}