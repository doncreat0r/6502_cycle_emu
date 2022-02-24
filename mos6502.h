#pragma once
#include <cstdint>

// 
// Virtual CPU pins - the only way CPU should talk to the outside world
//
struct Pins
{
    bool IRQ;
    bool NMI;
    bool RDY;
    bool RES;
    bool RW;
    bool SYNC;
    uint8_t PORT; // 6 bits
    uint8_t DATA;
    uint16_t ADDR;
};

enum class FLAGS6502 : uint8_t
{
    CF = (1 << 0),	// Carry
    ZF = (1 << 1),	// Zero
    IF = (1 << 2),	// Disable Interrupts
    DF = (1 << 3),	// BCD (Decimal) Mode
    BF = (1 << 4),	// Break
    XF = (1 << 5),	// Used internally
    VF = (1 << 6),	// Overflow
    NF = (1 << 7),	// Negative
};

class Flags6502 
{
    uint8_t _S; // flags byte

    class Flag
    {
        const FLAGS6502 M;
        uint8_t& S;
    public:
        Flag(uint8_t& x, const FLAGS6502 mask) : S(x), M(mask) {}
        operator bool() const { return S & static_cast<uint8_t>(M); }
        // not sure if S.C = true/false is better than S.C.Set() S.C.Clr()
        void Clr() { S &= ~static_cast<uint8_t>(M); }
        void Set() { S |= static_cast<uint8_t>(M); }
        Flag& operator=(bool const& f) { f ? Set() : Clr(); return *this; }
    };

public:
    // access individual flags in S register
    Flag C = { _S, FLAGS6502::CF };
    Flag Z = { _S, FLAGS6502::ZF };
    Flag I = { _S, FLAGS6502::IF };
    Flag D = { _S, FLAGS6502::DF };
    Flag B = { _S, FLAGS6502::BF };
    Flag X = { _S, FLAGS6502::XF };
    Flag V = { _S, FLAGS6502::VF };
    Flag N = { _S, FLAGS6502::NF };
    // register as is (for stack operations)
    Flags6502& operator=(uint8_t const& s) { _S = s; return *this; }
    operator uint8_t() const { return _S; }
};

struct Registers6502
{
    uint8_t A; // accumulator

    uint8_t X; // X index 
    uint8_t Y; // Y index

    uint8_t SP; // stack pointer
    uint16_t PC; // program counter
};

class mos6502
{
    // Helper class with raw registers, states and private methods access for debugging/monitoring purposes
    friend class Debugger;

private:
    typedef void (mos6502::* OpFunc)();

    struct OpData
    {
        const char* code;  // opcode text
        OpFunc addr;       // address access method reference (imm, zeropage, absolute, etc...)
        OpFunc func;       // function method reference (LDA, ORA, CMP, CLI, etc...)
        uint8_t cycles;    // number of cycles required to execute the instruction
    };
    //  CPU instructions jump table
    const OpData op_table[256] { 
        { "BRK", &mos6502::Addr_imp,      &mos6502::Op_BRK,         7 },  // 0x00 
        { "ORA", &mos6502::Addr_ind_X,    &mos6502::Op_ORA,         6 },  // 0x01
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x02
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x03
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x04
        { "ORA", &mos6502::Addr_zpg,      &mos6502::Op_ORA,         3 },  // 0x05
        { "ASL", &mos6502::Addr_zpg,      &mos6502::Op_ASL,         5 },  // 0x06
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x07
        { "PHP", &mos6502::Addr_imp,      &mos6502::Op_PHP,         3 },  // 0x08
        { "ORA", &mos6502::Addr_imm,      &mos6502::Op_ORA,         2 },  // 0x09
        { "ASL", &mos6502::Addr_imp,      &mos6502::Op_ASL_A,       2 },  // 0x0A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x0B
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x0C
        { "ORA", &mos6502::Addr_abs,      &mos6502::Op_ORA,         4 },  // 0x0D
        { "ASL", &mos6502::Addr_abs,      &mos6502::Op_ASL,         6 },  // 0x0E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x0F
        { "BPL", &mos6502::Addr_rel,      &mos6502::Op_BPL,         4 },  // 0x10
        { "ORA", &mos6502::Addr_ind_Y,    &mos6502::Op_ORA,         6 },  // 0x11
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x12
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x13
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x14
        { "ORA", &mos6502::Addr_zpg_X,    &mos6502::Op_ORA,         4 },  // 0x15
        { "ASL", &mos6502::Addr_zpg_X,    &mos6502::Op_ASL,         6 },  // 0x16
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x17
        { "CLC", &mos6502::Addr_imp,      &mos6502::Op_CLC,         2 },  // 0x18
        { "ORA", &mos6502::Addr_abs_Y,    &mos6502::Op_ORA,         5 },  // 0x19
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x1A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x1B
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x1C
        { "ORA", &mos6502::Addr_abs_X,    &mos6502::Op_ORA,         5 },  // 0x1D
        { "ASL", &mos6502::Addr_abs_X,    &mos6502::Op_ASL,         7 },  // 0x1E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x1F
        { "JSR", &mos6502::Addr_jsr,      &mos6502::Op_JSR,         6 },  // 0x20
        { "AND", &mos6502::Addr_ind_X,    &mos6502::Op_AND,         6 },  // 0x21
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x22
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x23
        { "BIT", &mos6502::Addr_zpg,      &mos6502::Op_BIT,         3 },  // 0x24
        { "AND", &mos6502::Addr_zpg,      &mos6502::Op_AND,         3 },  // 0x25
        { "ROL", &mos6502::Addr_zpg,      &mos6502::Op_ROL,         5 },  // 0x26
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x27
        { "PLP", &mos6502::Addr_imp,      &mos6502::Op_PLP,         4 },  // 0x28
        { "AND", &mos6502::Addr_imm,      &mos6502::Op_AND,         2 },  // 0x29
        { "ROL", &mos6502::Addr_imp,      &mos6502::Op_ROL_A,       2 },  // 0x2A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x2B
        { "BIT", &mos6502::Addr_abs,      &mos6502::Op_BIT,         4 },  // 0x2C
        { "AND", &mos6502::Addr_abs,      &mos6502::Op_AND,         4 },  // 0x2D
        { "ROL", &mos6502::Addr_abs,      &mos6502::Op_ROL,         6 },  // 0x2E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x2F
        { "BMI", &mos6502::Addr_rel,      &mos6502::Op_BMI,         4 },  // 0x30
        { "AND", &mos6502::Addr_ind_Y,    &mos6502::Op_AND,         6 },  // 0x31
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x32
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x33
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x34
        { "AND", &mos6502::Addr_zpg_X,    &mos6502::Op_AND,         7 },  // 0x35
        { "ROL", &mos6502::Addr_zpg_X,    &mos6502::Op_ROL,         7 },  // 0x36
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x37
        { "SEC", &mos6502::Addr_imp,      &mos6502::Op_SEC,         2 },  // 0x38
        { "AND", &mos6502::Addr_abs_Y,    &mos6502::Op_AND,         5 },  // 0x39
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x3A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x3B
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x3C
        { "AND", &mos6502::Addr_abs_X,    &mos6502::Op_AND,         5 },  // 0x3D
        { "ROL", &mos6502::Addr_abs_X,    &mos6502::Op_ROL,         7 },  // 0x3E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x3F
        { "RTI", &mos6502::Addr_imp,      &mos6502::Op_RTI,         6 },  // 0x40
        { "EOR", &mos6502::Addr_ind_X,    &mos6502::Op_EOR,         6 },  // 0x41
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x42
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x43
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x44
        { "EOR", &mos6502::Addr_zpg,      &mos6502::Op_EOR,         3 },  // 0x45
        { "LSR", &mos6502::Addr_zpg,      &mos6502::Op_LSR,         5 },  // 0x46
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x47
        { "PHA", &mos6502::Addr_imp,      &mos6502::Op_PHA,         3 },  // 0x48
        { "EOR", &mos6502::Addr_imm,      &mos6502::Op_EOR,         2 },  // 0x49
        { "LSR", &mos6502::Addr_imp,      &mos6502::Op_LSR_A,       2 },  // 0x4A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x4B
        { "JMP", &mos6502::Addr_abs,      &mos6502::Op_JMP,         3 },  // 0x4C
        { "EOR", &mos6502::Addr_abs,      &mos6502::Op_EOR,         4 },  // 0x4D
        { "LSR", &mos6502::Addr_abs,      &mos6502::Op_LSR,         6 },  // 0x4E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x4F
        { "BVC", &mos6502::Addr_rel,      &mos6502::Op_BVC,         4 },  // 0x50
        { "EOR", &mos6502::Addr_ind_Y,    &mos6502::Op_EOR,         6 },  // 0x51
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x52
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x53
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x54
        { "EOR", &mos6502::Addr_zpg_X,    &mos6502::Op_EOR,         4 },  // 0x55
        { "LSR", &mos6502::Addr_zpg_X,    &mos6502::Op_LSR,         6 },  // 0x56
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x57
        { "CLI", &mos6502::Addr_imp,      &mos6502::Op_CLI,         2 },  // 0x58
        { "EOR", &mos6502::Addr_abs_Y,    &mos6502::Op_EOR,         5 },  // 0x59
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x5A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x5B
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x5C
        { "EOR", &mos6502::Addr_abs_X,    &mos6502::Op_EOR,         5 },  // 0x5D
        { "LSR", &mos6502::Addr_abs_X,    &mos6502::Op_LSR,         7 },  // 0x5E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x5F
        { "RTS", &mos6502::Addr_imp,      &mos6502::Op_RTS,         6 },  // 0x60
        { "ADC", &mos6502::Addr_ind_X,    &mos6502::Op_ADC,         6 },  // 0x61
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x62
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x63
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x64
        { "ADC", &mos6502::Addr_zpg,      &mos6502::Op_ADC,         3 },  // 0x65
        { "ROR", &mos6502::Addr_zpg,      &mos6502::Op_ROR,         5 },  // 0x66
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x67
        { "PLA", &mos6502::Addr_imp,      &mos6502::Op_PLA,         4 },  // 0x68
        { "ADC", &mos6502::Addr_imm,      &mos6502::Op_ADC,         2 },  // 0x69
        { "ROR", &mos6502::Addr_imp,      &mos6502::Op_ROR_A,       2 },  // 0x6A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x6B
        { "JMP", &mos6502::Addr_ind,      &mos6502::Op_JMP,         5 },  // 0x6C
        { "ADC", &mos6502::Addr_abs,      &mos6502::Op_ADC,         4 },  // 0x6D
        { "ROR", &mos6502::Addr_abs,      &mos6502::Op_ROR,         6 },  // 0x6E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x6F
        { "BVS", &mos6502::Addr_rel,      &mos6502::Op_BVS,         4 },  // 0x70
        { "ADC", &mos6502::Addr_ind_Y,    &mos6502::Op_ADC,         6 },  // 0x71
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x72
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x73
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x74
        { "ADC", &mos6502::Addr_zpg_X,    &mos6502::Op_ADC,         4 },  // 0x75
        { "ROR", &mos6502::Addr_zpg_X,    &mos6502::Op_ROR,         6 },  // 0x76
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x77
        { "SEI", &mos6502::Addr_imp,      &mos6502::Op_SEI,         2 },  // 0x78
        { "ADC", &mos6502::Addr_abs_Y,    &mos6502::Op_ADC,         5 },  // 0x79
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x7A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x7B
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x7C
        { "ADC", &mos6502::Addr_abs_X,    &mos6502::Op_ADC,         5 },  // 0x7D
        { "ROR", &mos6502::Addr_abs_X,    &mos6502::Op_ROR,         7 },  // 0x7E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x7F
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x80
        { "STA", &mos6502::Addr_ind_X,    &mos6502::Op_STA,         6 },  // 0x81
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x82
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x83
        { "STY", &mos6502::Addr_zpg,      &mos6502::Op_STY,         3 },  // 0x84
        { "STA", &mos6502::Addr_zpg,      &mos6502::Op_STA,         3 },  // 0x85
        { "STX", &mos6502::Addr_zpg,      &mos6502::Op_STX,         3 },  // 0x86
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x87
        { "DEY", &mos6502::Addr_imp,      &mos6502::Op_DEY,         2 },  // 0x88
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x89
        { "TXA", &mos6502::Addr_imp,      &mos6502::Op_TXA,         2 },  // 0x8A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x8B
        { "STY", &mos6502::Addr_abs,      &mos6502::Op_STY,         4 },  // 0x8C
        { "STA", &mos6502::Addr_abs,      &mos6502::Op_STA,         4 },  // 0x8D
        { "STX", &mos6502::Addr_abs,      &mos6502::Op_STX,         4 },  // 0x8E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x8F
        { "BCC", &mos6502::Addr_rel,      &mos6502::Op_BCC,         4 },  // 0x90
        { "STA", &mos6502::Addr_ind_Y,    &mos6502::Op_STA,         6 },  // 0x91
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x92
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x93
        { "STY", &mos6502::Addr_zpg_X,    &mos6502::Op_STY,         4 },  // 0x94
        { "STA", &mos6502::Addr_zpg_X,    &mos6502::Op_STA,         4 },  // 0x95
        { "STX", &mos6502::Addr_zpg_Y,    &mos6502::Op_STX,         4 },  // 0x96
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x97
        { "TYA", &mos6502::Addr_imp,      &mos6502::Op_TYA,         2 },  // 0x98
        { "STA", &mos6502::Addr_abs_Y,    &mos6502::Op_STA,         5 },  // 0x99
        { "TXS", &mos6502::Addr_imp,      &mos6502::Op_TXS,         7 },  // 0x9A
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x9B
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x9C
        { "STA", &mos6502::Addr_abs_X,    &mos6502::Op_STA,         5 },  // 0x9D
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x9E
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0x9F
        { "LDY", &mos6502::Addr_imm,      &mos6502::Op_LDY,         2 },  // 0xA0
        { "LDA", &mos6502::Addr_ind_X,    &mos6502::Op_LDA,         6 },  // 0xA1
        { "LDX", &mos6502::Addr_imm,      &mos6502::Op_LDX,         2 },  // 0xA2
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xA3
        { "LDY", &mos6502::Addr_zpg,      &mos6502::Op_LDY,         3 },  // 0xA4
        { "LDA", &mos6502::Addr_zpg,      &mos6502::Op_LDA,         3 },  // 0xA5
        { "LDX", &mos6502::Addr_zpg,      &mos6502::Op_LDX,         3 },  // 0xA6
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xA7
        { "TAY", &mos6502::Addr_imp,      &mos6502::Op_TAY,         2 },  // 0xA8
        { "LDA", &mos6502::Addr_imm,      &mos6502::Op_LDA,         2 },  // 0xA9
        { "TAX", &mos6502::Addr_imp,      &mos6502::Op_TAX,         2 },  // 0xAA
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xAB
        { "LDY", &mos6502::Addr_abs,      &mos6502::Op_LDY,         4 },  // 0xAC
        { "LDA", &mos6502::Addr_abs,      &mos6502::Op_LDA,         4 },  // 0xAD
        { "LDX", &mos6502::Addr_abs,      &mos6502::Op_LDX,         4 },  // 0xAE
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xAF
        { "BCS", &mos6502::Addr_rel,      &mos6502::Op_BCS,         4 },  // 0xB0
        { "LDA", &mos6502::Addr_ind_Y,    &mos6502::Op_LDA,         6 },  // 0xB1
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xB2
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xB3
        { "LDY", &mos6502::Addr_zpg_X,    &mos6502::Op_LDY,         4 },  // 0xB4
        { "LDA", &mos6502::Addr_zpg_X,    &mos6502::Op_LDA,         4 },  // 0xB5
        { "LDX", &mos6502::Addr_zpg_Y,    &mos6502::Op_LDX,         4 },  // 0xB6
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xB7
        { "CLV", &mos6502::Addr_imp,      &mos6502::Op_CLV,         2 },  // 0xB8
        { "LDA", &mos6502::Addr_abs_Y,    &mos6502::Op_LDA,         5 },  // 0xB9
        { "TSX", &mos6502::Addr_imp,      &mos6502::Op_TSX,         2 },  // 0xBA
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xBB
        { "LDY", &mos6502::Addr_abs_X,    &mos6502::Op_LDY,         5 },  // 0xBC
        { "LDA", &mos6502::Addr_abs_X,    &mos6502::Op_LDA,         5 },  // 0xBD
        { "LDX", &mos6502::Addr_abs_Y,    &mos6502::Op_LDX,         5 },  // 0xBE
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xBF
        { "CPY", &mos6502::Addr_imm,      &mos6502::Op_CPY,         2 },  // 0xC0
        { "CMP", &mos6502::Addr_ind_X,    &mos6502::Op_CMP,         6 },  // 0xC1
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xC2
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xC3
        { "CPY", &mos6502::Addr_zpg,      &mos6502::Op_CPY,         3 },  // 0xC4
        { "CMP", &mos6502::Addr_zpg,      &mos6502::Op_CMP,         3 },  // 0xC5
        { "DEC", &mos6502::Addr_zpg,      &mos6502::Op_DEC,         5 },  // 0xC6
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xC7
        { "INY", &mos6502::Addr_imp,      &mos6502::Op_INY,         2 },  // 0xC8
        { "CMP", &mos6502::Addr_imm,      &mos6502::Op_CMP,         2 },  // 0xC9
        { "DEX", &mos6502::Addr_imp,      &mos6502::Op_DEX,         2 },  // 0xCA
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xCB
        { "CPY", &mos6502::Addr_abs,      &mos6502::Op_CPY,         4 },  // 0xCC
        { "CMP", &mos6502::Addr_abs,      &mos6502::Op_CMP,         4 },  // 0xCD
        { "DEC", &mos6502::Addr_abs,      &mos6502::Op_DEC,         6 },  // 0xCE
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xCF
        { "BNE", &mos6502::Addr_rel,      &mos6502::Op_BNE,         4 },  // 0xD0
        { "CMP", &mos6502::Addr_ind_Y,    &mos6502::Op_CMP,         6 },  // 0xD1
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xD2
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xD3
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xD4
        { "CMP", &mos6502::Addr_zpg_X,    &mos6502::Op_CMP,         4 },  // 0xD5
        { "DEC", &mos6502::Addr_zpg_X,    &mos6502::Op_DEC,         6 },  // 0xD6
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xD7
        { "CLD", &mos6502::Addr_imp,      &mos6502::Op_CLD,         2 },  // 0xD8
        { "CMP", &mos6502::Addr_abs_Y,    &mos6502::Op_CMP,         5 },  // 0xD9
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xDA
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xDB
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xDC
        { "CMP", &mos6502::Addr_abs_X,    &mos6502::Op_CMP,         5 },  // 0xDD
        { "DEC", &mos6502::Addr_abs_X,    &mos6502::Op_DEC,         7 },  // 0xDE
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xDF
        { "CPX", &mos6502::Addr_imm,      &mos6502::Op_CPX,         2 },  // 0xE0
        { "SBC", &mos6502::Addr_ind_X,    &mos6502::Op_SBC,         6 },  // 0xE1
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xE2
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xE3
        { "CPX", &mos6502::Addr_zpg,      &mos6502::Op_CPX,         3 },  // 0xE4
        { "SBC", &mos6502::Addr_zpg,      &mos6502::Op_SBC,         3 },  // 0xE5
        { "INC", &mos6502::Addr_zpg,      &mos6502::Op_INC,         5 },  // 0xE6
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xE7
        { "INX", &mos6502::Addr_imp,      &mos6502::Op_INX,         2 },  // 0xE8
        { "SBC", &mos6502::Addr_imm,      &mos6502::Op_SBC,         2 },  // 0xE9
        { "NOP", &mos6502::Addr_imp,      &mos6502::Op_NOP,         2 },  // 0xEA
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xEB
        { "CPX", &mos6502::Addr_abs,      &mos6502::Op_CPX,         4 },  // 0xEC
        { "SBC", &mos6502::Addr_abs,      &mos6502::Op_SBC,         4 },  // 0xED
        { "INC", &mos6502::Addr_abs,      &mos6502::Op_INC,         6 },  // 0xEE
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xEF
        { "BEQ", &mos6502::Addr_rel,      &mos6502::Op_BEQ,         4 },  // 0xF0
        { "SBC", &mos6502::Addr_ind_Y,    &mos6502::Op_SBC,         6 },  // 0xF1
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xF2
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xF3
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xF4
        { "SBC", &mos6502::Addr_zpg_X,    &mos6502::Op_SBC,         4 },  // 0xF5
        { "INC", &mos6502::Addr_zpg_X,    &mos6502::Op_INC,         6 },  // 0xF6
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xF7
        { "SED", &mos6502::Addr_imp,      &mos6502::Op_SED,         2 },  // 0xF8
        { "SBC", &mos6502::Addr_abs_Y,    &mos6502::Op_SBC,         5 },  // 0xF9
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xFA
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xFB
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 },  // 0xFC
        { "SBC", &mos6502::Addr_abs_X,    &mos6502::Op_SBC,         5 },  // 0xFD
        { "INC", &mos6502::Addr_abs_X,    &mos6502::Op_INC,         7 },  // 0xFE
        { "BAD", &mos6502::Addr_imp,      &mos6502::Op_BAD,         2 }   // 0xFF
    };

    //// Registers
    Registers6502 R;

    uint8_t AD;  // internal data register
    uint16_t AR; // internal address register

    uint8_t ticks; // ticks to handle current instruction
    uint8_t ticks_func; // ticks spent doing function part after addressing is done
    uint8_t Opcode; // current instruction byte
    uint64_t ticks_total; // total number of ticks per emulation session

    Flags6502 S;  // CPU flags (accessed R/W by bits and entire byte)

    // 16-bit fixed system vectors, accessed by CPU
    static const uint16_t nmiVectorL = 0xFFFA;
    static const uint16_t nmiVectorH = 0xFFFB;
    static const uint16_t rstVectorL = 0xFFFC;
    static const uint16_t rstVectorH = 0xFFFD;
    static const uint16_t irqVectorL = 0xFFFE;
    static const uint16_t irqVectorH = 0xFFFF;
    // stack base address
    static const uint16_t stackBase = 0x100;

    bool addressing_done;  // set by Addr_xxx after all addressing ticks are completed to start Op_xxx calling
    // interrupt signals to activate after finishing executing pending instruction
    bool NMI_signal;
    bool IRQ_signal;

    // bad opcode 
    void Op_BAD() { NextOp(); }
    // reset operation - init PC from reset vector in 7 ticks
    void Op_RES();

    // Addressing modes
    void Addr_imp();            // implicit
    void Addr_rel();            // relative (for branching) - all ticks are handled by Op_Bxx for now!
    void Addr_imm();            // immediate value in the code
    void Addr_ind_X();          // address at zero page + X
    void Addr_ind_Y();          // address at zero page + Y
    void Addr_zpg();            // in zero page
    void Addr_zpg_X();          // via zero page + X
    void Addr_zpg_Y();          // via zero page + Y
    void Addr_abs();            // absolute 16-bit address in the code
    void Addr_jsr();            // absolute addressing for JSR - all ticks are handled by Op_JSR for now!
    void Addr_ind();            // absolute indirect 16-bit address (for JMP)
    void Addr_abs_X();          // absolute 16-bit address in the code + X
    void Addr_abs_Y();          // absolute 16-bit address in the code + Y

    // Main opcodes
    void Op_BRK();              // 0x00
    
    void Op_ORA();              // 0x01, 0x11, 0x05, 0x15, 0x09, 0x0D, 0x1D, 0x19

    void Op_ASL();              // 0x06, 0x16, 0x0E, 0x1E
    void Op_ASL_A();            // 0x0A

    void Op_AND();              // 0x21, 0x31, 0x25, 0x35, 0x29, 0x2D, 0x3D, 0x39

    void Op_ROL();              // 0x26, 0x36, 0x2E, 0x3E
    void Op_ROL_A();            // 0x2A

    void Op_EOR();              // 0x41, 0x51, 0x45, 0x55, 0x49, 0x4D, 0x5D, 0x59

    void Op_LSR();              // 0x46, 0x56, 0x4E, 0x5E
    void Op_LSR_A();            // 0x4A

    void Op_ADC();              // 0x61, 0x71, 0x65, 0x75, 0x69, 0x6D, 0x7D, 0x79

    void Op_ROR();              // 0x66, 0x76, 0x6E, 0x7E
    void Op_ROR_A();            // 0x6A

    void Op_STA();              // 0x81, 0x91, 0x85, 0x95, 0x8D, 0x9D, 0x99
    void Op_STX();              // 0x86, 0x96, 0x8E
    void Op_STY();              // 0x84, 0x94, 0x8C

    void Op_LDA();              // 0xA1, 0xB1, 0xA5, 0xB5, 0xAD, 0x9D, 0xA9, 0xB9
    void Op_LDX();              // 0xA2, 0xA6, 0xB6, 0xAE, 0xBE
    void Op_LDY();              // 0xA0, 0xA4, 0xB4, 0xAC, 0xBC

    void Op_CMP();              // 0xC1, 0xD1, 0xC5, 0xD5, 0xCD, 0xDD, 0xC9, 0xD9
    void Op_DEC();              // 0xC6, 0xD6, 0xCE, 0xDE
    void Op_CPY();              // 0xC0, 0xC4, 0xCC
    void Op_SBC();              // 0xE1, 0xF1, 0xE5, 0xF5, 0xED, 0xFD, 0xE9, 0xF9
    void Op_INC();              // 0xE6, 0xF6, 0xEE, 0xFE
    void Op_CPX();              // 0xE0, 0xF4, 0xEC

    void Op_BIT();              // 0x24, 0x2C

    // Register A operations & no-operand operations
    void Op_PHP();              // 0x08
    void Op_CLC();              // 0x18
    void Op_PLP();              // 0x28
    void Op_SEC();              // 0x38
    void Op_PHA();              // 0x48
    void Op_CLI();              // 0x58
    void Op_PLA();              // 0x68
    void Op_SEI();              // 0x78
    void Op_DEY();              // 0x88
    void Op_TYA();              // 0x98
    void Op_TAY();              // 0xA8
    void Op_CLV();              // 0xB8
    void Op_INY();              // 0xC8
    void Op_CLD();              // 0xD8
    void Op_INX();              // 0xE8
    void Op_SED();              // 0xF8
    void Op_TXA();              // 0x8A
    void Op_TXS();              // 0x9A
    void Op_TAX();              // 0xAA
    void Op_TSX();              // 0xBA
    void Op_DEX();              // 0xCA
    void Op_NOP();              // 0xEA

    // Jumps, calls & rets
    void Op_JSR();              // 0x20
    void Op_JMP();              // 0x4C, 0x6C
    void Op_RTI();              // 0x40
    void Op_RTS();              // 0x60

    // Branches
    void Do_Branch(bool skip);  // common branching function
    void Op_BPL();              // 0x10
    void Op_BMI();              // 0x30
    void Op_BVC();              // 0x50
    void Op_BVS();              // 0x70
    void Op_BCC();              // 0x90
    void Op_BCS();              // 0xB0
    void Op_BNE();              // 0xD0
    void Op_BEQ();              // 0xF0

    // Internal functions
    inline void UpdateNZ(uint8_t v);
    inline void NextOp();

    inline void AND_A_Flags(uint8_t v);
    inline uint8_t ASL_Flags(uint8_t v);
    inline uint8_t LSR_Flags(uint8_t v);
    inline uint8_t ROL_Flags(uint8_t v);
    inline uint8_t ROR_Flags(uint8_t v);
    inline void ADC_Flags(uint8_t v);
    inline void SBC_Flags(uint8_t v);
    inline void CMP_Flags(uint8_t r, uint8_t v);
    // 
    Pins _pins;

public:
    mos6502();
    ~mos6502();

    Pins reset();

    Pins tick(Pins pins);

    // some debugging and low level CPU control
    uint16_t readPC() { return R.PC; }
    uint8_t readSP() { return R.SP; }
    uint64_t readTicksTotal() { return ticks_total; }
    Pins forceJumpTo(uint16_t pc) { 
        _pins.ADDR = pc;
        R.PC = pc;
        _pins.SYNC = true;
        return _pins;
    }
    void Halt() { _pins.RDY = true; }

};

