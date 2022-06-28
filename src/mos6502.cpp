#include "mos6502.h"

void mos6502::Op_RES()
{
    switch (ticks)
    {        
    case 1: _pins.ADDR = rstVectorL; break; // read address of the reset vector
    case 2: AR = _pins.DATA; break;         // ...
    case 3: _pins.ADDR = rstVectorH; break; // ...
    case 4: AR |= (_pins.DATA << 8); break; // ...
    case 5: R.PC = AR; break;                 // update PC with a vector
    case 6: _pins.ADDR = R.PC; break;         // init address bus with PC
    case 7: _pins.RES = false; break;       // finish reset state
    }
    ticks++; // force ticks increment here because the rest of tick() is skipped when CPU is in the reset state
}

void mos6502::Addr_imp()
{
    addressing_done = true;
}

void mos6502::Addr_rel()
{
    addressing_done = true;
}

void mos6502::Addr_imm()
{
    if (ticks == 0) {
        _pins.ADDR = R.PC++;
        addressing_done = true;
    }
}

void mos6502::Addr_ind_X()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: AR = _pins.DATA; _pins.ADDR = AR; break;
    case 2: AR = (AR + R.X) & 0xFF; _pins.ADDR = AR; break;
    case 3: _pins.ADDR = (AR + 1) & 0xFF; AR = _pins.DATA; break;
    case 4: 
        _pins.ADDR = (_pins.DATA << 8) + AR; 
        addressing_done = true; 
        break;
    }
}

void mos6502::Addr_ind_Y()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: AR = _pins.DATA; _pins.ADDR = AR; break;
    case 2: _pins.ADDR = (AR + 1) & 0xFF; AR = _pins.DATA; break;
    case 3:
        AR |= (_pins.DATA << 8); _pins.ADDR = (AR & 0xFF00) + ((AR + R.Y) & 0xFF);
        if ((AR >> 8) >= ((AR + R.Y) >> 8))
        {
            ticks++; // skip tick if no page crossing
            addressing_done = true;
        }
        break;
    case 4: 
        _pins.ADDR = AR + R.Y; 
        addressing_done = true; 
        break; // read with page crossing
    }
}

void mos6502::Addr_zpg()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: 
        _pins.ADDR = _pins.DATA; 
        addressing_done = true;
        break;
    }
}

void mos6502::Addr_zpg_X()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: AR = _pins.DATA; _pins.ADDR = AR; break;
    case 2: 
        _pins.ADDR = (AR + R.X) & 0xFF; 
        addressing_done = true;
        break;
    }
}

void mos6502::Addr_zpg_Y()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: AR = _pins.DATA; _pins.ADDR = AR; break;
    case 2:
        _pins.ADDR = (AR + R.Y) & 0xFF;
        addressing_done = true;
        break;
    }
}

void mos6502::Addr_abs()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: _pins.ADDR = R.PC++; AR = _pins.DATA; break;
    case 2: 
        _pins.ADDR = (_pins.DATA << 8) + AR; 
        addressing_done = true;
        break;
    }
}

void mos6502::Addr_jsr()
{
    addressing_done = true;
}

void mos6502::Addr_ind()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: _pins.ADDR = R.PC++; AR = _pins.DATA; break;
    case 2: AR |= (_pins.DATA << 8); _pins.ADDR = AR; break;
    case 3: _pins.ADDR = (AR & 0xFF00) + ((AR + 1) & 0xFF); AR = _pins.DATA; break;
    case 4: 
        _pins.ADDR = (_pins.DATA << 8) + AR;
        addressing_done = true;
        break;
    }
}

void mos6502::Addr_abs_X()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: _pins.ADDR = R.PC++; AR = _pins.DATA; break;
    case 2:
        AR |= (_pins.DATA << 8); _pins.ADDR = (AR & 0xFF00) + ((AR + R.X) & 0xFF);
        if ((AR >> 8) >= ((AR + R.X) >> 8))
        {
            ticks++; // skip tick if no page crossing
            addressing_done = true;
        }
        break;
    case 3: 
        _pins.ADDR = AR + R.X; 
        addressing_done = true;
        break; // read with page crossing
    }
}

void mos6502::Addr_abs_Y()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: _pins.ADDR = R.PC++; AR = _pins.DATA; break;
    case 2:
        AR |= (_pins.DATA << 8); _pins.ADDR = (AR & 0xFF00) + ((AR + R.Y) & 0xFF);
        if ((AR >> 8) >= ((AR + R.Y) >> 8))
        {
            ticks++; // skip tick if no page crossing
            addressing_done = true;
        }
        break;
    case 3: 
        _pins.ADDR = AR + R.Y; 
        addressing_done = true;
        break; // read with page crossing
    }
}

// used as BRK instruction and also as a forced instruction when NMI or IRQ pin is triggered
void mos6502::Op_BRK()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: 
        if (!IRQ_signal && !NMI_signal) R.PC++;
        _pins.ADDR = stackBase + R.SP--;
        _pins.DATA = (R.PC >> 8);
        if (!_pins.RES) _pins.RW = false;  // don't write anything in reset state
        break;
    case 2:
        _pins.ADDR = stackBase + R.SP--;
        _pins.DATA = R.PC & 0xFF;
        if (!_pins.RES) _pins.RW = false;  // don't write anything in reset state
        break;
    case 3:
        _pins.ADDR = stackBase + R.SP--;
        _pins.DATA = S | static_cast<uint8_t>(FLAGS6502::XF);
        if (_pins.RES)
            AR = rstVectorL;
        else
        {
            _pins.RW = false;
            AR = NMI_signal ? nmiVectorL : irqVectorL;
        }
        break;
    case 4: _pins.ADDR = AR++; S.I = true; S.B = true; NMI_signal = false; IRQ_signal = false; break;
    case 5: _pins.ADDR = AR; AR = _pins.DATA; break;
    case 6: R.PC = (_pins.DATA << 8) + AR; break;
    }
}

void mos6502::Op_ORA()
{
    switch (ticks_func)
    {
    case 0: break; // overlaps with addressing ticks!
    case 1: R.A |= _pins.DATA; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_ASL()
{
    switch (ticks_func)
    {
    case 0: break;
    case 1: AD = _pins.DATA; _pins.RW = false; break;
    case 2: _pins.DATA = ASL_Flags(AD); _pins.RW = false; break;
    case 3: break; // NextOp();
    }
}

void mos6502::Op_ASL_A()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.A = ASL_Flags(R.A); break;
    }
}

void mos6502::Op_AND()
{
    switch (ticks_func)
    {
    case 0: break;
    case 1: R.A &= _pins.DATA; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_ROL()
{
    switch (ticks_func)
    {
    case 1: AD = _pins.DATA; _pins.RW = false; break;
    case 2: _pins.DATA = ROL_Flags(AD); _pins.RW = false; break;
    case 3: break; // NextOp();
    }
}

void mos6502::Op_ROL_A()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.A = ROL_Flags(R.A); break;
    }
}

void mos6502::Op_EOR()
{
    switch (ticks_func)
    {
    case 1: R.A ^= _pins.DATA; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_LSR()
{
    switch (ticks_func)
    {
    case 1: AD = _pins.DATA; _pins.RW = false; break;
    case 2: _pins.DATA = LSR_Flags(AD); _pins.RW = false; break;
    case 3: break; // NextOp();
    }
}

void mos6502::Op_LSR_A()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.A = LSR_Flags(R.A); break;
    }
}

void mos6502::Op_ADC()
{
    switch (ticks_func)
    {
    case 1: ADC_Flags(_pins.DATA);
    }
}

void mos6502::Op_ROR()
{
    switch (ticks_func)
    {
    case 1: AD = _pins.DATA; _pins.RW = false; break;
    case 2: _pins.DATA = ROR_Flags(AD); _pins.RW = false; break;
    case 3: break; // NextOp();
    }
}

void mos6502::Op_ROR_A()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.A = ROR_Flags(R.A); break;
    }
}

void mos6502::Op_STA()
{
    switch (ticks_func)
    {
    case 0: _pins.DATA = R.A; _pins.RW = false; break; // Overlaps with addressing!
    case 1: break; // NextOp();
    }
}

void mos6502::Op_STX()
{
    switch (ticks_func)
    {
    case 0: _pins.DATA = R.X; _pins.RW = false; break; // Overlaps with addressing!
    case 1: break; // NextOp();
    }
}

void mos6502::Op_STY()
{
    switch (ticks_func)
    {
    case 0: _pins.DATA = R.Y; _pins.RW = false; break; // Overlaps with addressing!
    case 1: break; // NextOp();
    }
}

void mos6502::Op_LDA()
{
    switch (ticks_func)
    {
    case 1: R.A = _pins.DATA; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_LDX()
{
    switch (ticks_func)
    {
    case 1: R.X = _pins.DATA; UpdateNZ(R.X); break;
    }
}

void mos6502::Op_LDY()
{
    switch (ticks_func)
    {
    case 1: R.Y = _pins.DATA; UpdateNZ(R.Y); break;
    }
}

void mos6502::Op_CMP()
{
    switch (ticks_func)
    {
    case 1: CMP_Flags(R.A, _pins.DATA); break;
    }
}

void mos6502::Op_DEC()
{
    switch (ticks_func)
    {
    case 1: AD = _pins.DATA; _pins.RW = false; break;
    case 2: AD--; UpdateNZ(AD); _pins.DATA = AD; _pins.RW = false; break;
    case 3: break; // NextOp();
    }
}

void mos6502::Op_CPY()
{
    switch (ticks_func)
    {
    case 1: CMP_Flags(R.Y, _pins.DATA); break;
    }
}

void mos6502::Op_SBC()
{
    switch (ticks_func)
    {
    case 1: SBC_Flags(_pins.DATA); break;
    }
}

void mos6502::Op_INC()
{
    switch (ticks_func)
    {
    case 1: AD = _pins.DATA; _pins.RW = false; break;
    case 2: AD++; UpdateNZ(AD); _pins.DATA = AD; _pins.RW = false; break;
    case 3: break; // NextOp();
    }
}

void mos6502::Op_CPX()
{
    switch (ticks_func)
    {
    case 1: CMP_Flags(R.X, _pins.DATA); break;
    }
}

void mos6502::Op_BIT()
{
    switch (ticks_func)
    {
    case 1: AND_A_Flags(_pins.DATA); break;
    }
}

void mos6502::Op_PHP()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    //case 1: _pins.ADDR = stackBase + R.SP--; S.X = true; _pins.DATA = S; _pins.RW = false; break;
    case 1: _pins.ADDR = stackBase + R.SP--; _pins.DATA = S | static_cast<uint8_t>(FLAGS6502::XF); _pins.RW = false; break;
    case 2: break; // NextOp();
    }
}

void mos6502::Op_CLC()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.C = false; break;
    }
}

void mos6502::Op_PLP()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: _pins.ADDR = stackBase + R.SP++; break;
    case 2: _pins.ADDR = stackBase + R.SP; break;
    case 3: S = _pins.DATA; S.B = true; S.X = false; break;
    }
}

void mos6502::Op_SEC()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.C = true; break;
    }
}

void mos6502::Op_PHA()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: _pins.ADDR = stackBase + R.SP--; _pins.DATA = R.A; _pins.RW = false; break;
    case 2: break; // NextOp();
    }
}

void mos6502::Op_CLI()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.I = false; break;
    }
}

void mos6502::Op_PLA()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: _pins.ADDR = stackBase + R.SP++; break;
    case 2: _pins.ADDR = stackBase + R.SP; break;
    case 3: R.A = _pins.DATA; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_SEI()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.I = true; break;
    }
}

void mos6502::Op_DEY()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.Y--; UpdateNZ(R.Y); break;
    }
}

void mos6502::Op_TYA()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.A = R.Y; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_TAY()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.Y = R.A; UpdateNZ(R.Y); break;
    }
}

void mos6502::Op_CLV()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.V = false; break;
    }
}

void mos6502::Op_INY()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.Y++; UpdateNZ(R.Y); break;
    }
}

void mos6502::Op_CLD()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.D = false; break;
    }
}

void mos6502::Op_INX()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.X++; UpdateNZ(R.X); break;
    }
}

void mos6502::Op_SED()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: S.D = true; break;
    }
}

void mos6502::Op_TXA()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.A = R.X; UpdateNZ(R.A); break;
    }
}

void mos6502::Op_TXS()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.SP = R.X; break;
    }
}

void mos6502::Op_TAX()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.X = R.A; UpdateNZ(R.X); break;
    }
}

void mos6502::Op_TSX()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.X = R.SP; UpdateNZ(R.X); break;
    }
}

void mos6502::Op_DEX()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: R.X--; UpdateNZ(R.X); break;
    }
}

void mos6502::Op_NOP()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    }
}

void mos6502::Op_JSR()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1: _pins.ADDR = stackBase + R.SP; AR = _pins.DATA; break;
    case 2: _pins.ADDR = stackBase + R.SP--; _pins.DATA = (R.PC >> 8); _pins.RW = false; break;
    case 3: _pins.ADDR = stackBase + R.SP--; _pins.DATA = R.PC & 0xFF; _pins.RW = false; break;
    case 4: _pins.ADDR = R.PC; break;
    case 5: R.PC = (_pins.DATA << 8) + AR; break;
    }
}

void mos6502::Op_JMP()
{
    switch (ticks_func)
    {
    case 0: R.PC = (_pins.DATA << 8) + AR; break;
    }
}

void mos6502::Op_RTI()
{
    switch (ticks) {
    case 0: _pins.ADDR = R.PC; break;
    case 1: _pins.ADDR = stackBase + R.SP++; break;
    case 2: _pins.ADDR = stackBase + R.SP++; break;
    case 3: _pins.ADDR = stackBase + R.SP++; S = _pins.DATA; S.B = true; S.X = false; break;
    case 4: _pins.ADDR = stackBase + R.SP; AR = _pins.DATA; break;
    case 5: R.PC = (_pins.DATA << 8) + AR; break;
    }
}

void mos6502::Op_RTS()
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC; break;
    case 1: _pins.ADDR = stackBase + R.SP++; break;
    case 2: _pins.ADDR = stackBase + R.SP++; break;
    case 3: _pins.ADDR = stackBase + R.SP; AR = _pins.DATA; break;
    case 4: R.PC = (_pins.DATA << 8) + AR; _pins.ADDR = R.PC++; break;
    case 5: break; // NextOp();
    }
}

void mos6502::Do_Branch(bool skip)
{
    switch (ticks)
    {
    case 0: _pins.ADDR = R.PC++; break;
    case 1:
        _pins.ADDR = R.PC;
        AR = R.PC + (int8_t)_pins.DATA; // treat relative address as signed
        if (skip) ticks = 4; // if negative - skip
        break;
    case 2:
        _pins.ADDR = (R.PC & 0xFF00) + (AR & 0xFF);
        if ((R.PC & 0xFF00) == (AR & 0xFF00))
        {
            R.PC = AR;
            ticks = 4;
        }
        break;
    case 3: R.PC = AR; break;
    }
}

void mos6502::Op_BPL()
{
    Do_Branch(S.N);
}

void mos6502::Op_BMI()
{
    Do_Branch(!S.N);
}

void mos6502::Op_BVC()
{
    Do_Branch(S.V);
}

void mos6502::Op_BVS()
{
    Do_Branch(!S.V);
}

void mos6502::Op_BCC()
{
    Do_Branch(S.C);
}

void mos6502::Op_BCS()
{
    Do_Branch(!S.C);
}

void mos6502::Op_BNE()
{
    Do_Branch(S.Z);
}

void mos6502::Op_BEQ()
{
    Do_Branch(!S.Z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void mos6502::UpdateNZ(uint8_t v)
{
    S.Z = (!v);
    S.N = (v & 0x80);
}

inline void mos6502::AND_A_Flags(uint8_t v)
{
    uint8_t test = R.A & v;
    S.Z = (!test);
    S.N = (v & 0x80);
    S.V = (v & 0x40);
}

inline uint8_t mos6502::ASL_Flags(uint8_t v)
{
    uint8_t t;
    S.C = (v & 0x80);
    t = (v << 1) & 0xFF;
    UpdateNZ(t);
    return t;
}

inline uint8_t mos6502::LSR_Flags(uint8_t v)
{
    uint8_t t = (v >> 1);
    S.C = (v & 0x01);
    UpdateNZ(t);
    return t;
}

inline uint8_t mos6502::ROL_Flags(uint8_t v)
{
    bool carry = S.C;
    S.C = (v & 0x80);
    v <<= 1;
    if (carry) v |= 1;
    UpdateNZ(v);
    return v;
}

inline uint8_t mos6502::ROR_Flags(uint8_t v)
{
    bool carry = S.C;
    S.C = (v & 0x01);
    v >>= 1;
    if (carry) v |= 0x80;
    UpdateNZ(v);
    return v;
}

inline void mos6502::ADC_Flags(uint8_t v)
{
    uint16_t sum = R.A + v + (S.C ? 1 : 0);  // can feed bool as int directly, but...
    S.Z = !(sum & 0xFF);

    if (S.D) {
        if (((R.A & 0xF) + (v & 0xF) + (S.C ? 1 : 0)) > 9) sum += 6;
        S.N = (sum & 0x80);
        S.V = (!((R.A ^ v) & 0x80) && ((R.A ^ sum) & 0x80));
        if (sum > 0x99)
        {
            sum += 96;
        }
        S.C = (sum> 0x99);
    }
    else 
    {
        S.N = (sum & 0x80);
        S.V = (!((R.A ^ v) & 0x80) && ((R.A ^ sum) & 0x80));
        S.C = (sum > 0xFF);
    }
    R.A = sum & 0xFF;
}

inline void mos6502::SBC_Flags(uint8_t v)
{
    uint16_t dif = R.A - v - (S.C ? 0 : 1);
    S.N = (dif & 0x80);
    S.Z = !(dif & 0xFF);
    S.V = ((R.A ^ dif) & 0x80) && ((R.A ^ v) & 0x80);

    if (S.D)
    {
        if (((R.A & 0x0F) - (S.C ? 0 : 1)) < (v & 0x0F)) dif -= 6;
        if (dif > 0x99)
        {
            dif -= 0x60;
        }
    }
    S.C = (dif < 0x100);
    R.A = (dif & 0xFF);
}

inline void mos6502::CMP_Flags(uint8_t r, uint8_t v)
{
    uint16_t t = r - v;
    S.C = (r >= v);
    S.Z = (!t);
    S.N = (t & 0x80);
}

mos6502::mos6502()
{
    _pins = reset();
    _pins.ADDR = 0;
    _pins.DATA = 0;
    _pins.IRQ = false;
    _pins.NMI = false;
    _pins.PORT = 0;
    _pins.RDY = false;
    _pins.RES = true;
    _pins.RW = true;
    _pins.SYNC = true;
}

mos6502::~mos6502()
{
    // I'm the destroyer of the worlds!!!
}

Pins mos6502::reset()
{
    R.A = 0;
    R.X = 0;
    R.Y = 0;
    R.PC = 0; 
    R.SP = 0xFD;
    S.N = false;
    S.V = false;
    S.X = true;
    S.B = false;
    S.D = false;
    S.I = false;
    S.Z = false;
    S.C = false;
    NMI_signal = false;
    IRQ_signal = false;
    ticks = 0;
    ticks_func = 0;
    addressing_done = false;
    return _pins;
}

Pins mos6502::tick(Pins pins)
{
    // edge detect NMI
    if (!_pins.NMI && pins.NMI)
        NMI_signal = true;
    // level detect IRQ
    if (pins.IRQ && !S.I)
        IRQ_signal = true;

    _pins = pins;

    // if RDY is high & RW is "read" - skip op execution
    if (_pins.RDY && _pins.RW)
        return _pins;

    if (_pins.SYNC || _pins.IRQ || _pins.NMI || _pins.RES)
    {
        // process reset
        if (_pins.RES)
        {
            Op_RES();
            return _pins;
        }

        // fetch a new instruction byte from DATA pins while SYNC is set
        if (_pins.SYNC)
        {
            Opcode = _pins.DATA;
            _pins.SYNC = false;

            // process the interrupt by simulating the BRK instruction
            if (NMI_signal || IRQ_signal)
            {
                Opcode = 0;
                S.B = false;
                _pins.RES = false;
            }
            else
            {
                R.PC++;
            }
            ticks = 0;
            ticks_func = 0;
            addressing_done = false;
        }
    } // if some pins

    // set RW pin by default as read mode on the bus
    _pins.RW = true;
    // handle the instruction by its addressing mode and operation code as an index in the jump table
    // call the func right away if addressing_done is set to allow overlap addr & func for one tick
    if (!addressing_done)
    {
        (this->*(op_table[Opcode].addr))();
    }
    if (addressing_done)
    {
        (this->*(op_table[Opcode].func))();
        ticks_func++;
    }
    if (ticks + 1 >= this->op_table[Opcode].cycles) 
        NextOp();
    else
        ticks++;

    ticks_total++;
    return _pins;
}


