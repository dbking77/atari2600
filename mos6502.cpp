#include "mos6502.hpp"

#include <iomanip>
#include <sstream>

Mos6502::Mos6502(Mos6502::ReadCallback read_callback, Mos6502::WriteCallback write_callback) :
  read_callback_{read_callback},
  write_callback_{write_callback}
{
  for (auto& op_info : op_table_)
  {
    op_info.func = nullptr;
    op_info.name = nullptr;
    op_info.len = 0;
  }

  addArithmeticInstructions();
  addLoadInstructions();
  addStoreInstructions();
  addTransferInstructions();
  addSpecialInstructions();
  addBranchInstructions();
  addStackInstructions();
  addCompareInstructions();
  addShiftAndRotateInstructions();
  addLogicalInstructions();

  OpFunc invalid_func = [](Mos6502& cpu) -> unsigned
  {
    std::ostringstream ss;
    ss << "Invalid Instr " << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(cpu.instr_[0]);
    throw std::runtime_error(ss.str());
    return 1;
  };

  unsigned op_code_count = 0;
  for (auto& op_info : op_table_)
  {
    if (op_info.func == nullptr)
    {
      op_info = OpInfo{"<?>", invalid_func, 1};
    }
    else
    {
      ++op_code_count;
    }
  }
  std::cout << "OpCode Count " << op_code_count << std::endl;
}

void Mos6502::addInstruction(uint8_t opcode, const char* op_name, uint8_t op_len, OpFunc op_func)
{
  auto& op_info = op_table_.at(opcode);
  if ((op_info.func != nullptr) and (op_info.name != nullptr))
  {
    std::ostringstream ss;
    ss << "repeat instruction with opcode "
      << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(opcode) << std::dec
      << " prev op " << op_info.name;
    throw std::runtime_error(ss.str());
  }
  op_table_.at(opcode) = {op_name, op_func, op_len};
}

uint8_t Mos6502::loadImmediateNZ()
{
  updateNZ(instr_[1]);
  return instr_[1];
}

uint8_t Mos6502::loadZeroPage()
{
  uint16_t addr = instr_[1];
  return read_callback_(addr);
}

uint8_t Mos6502::loadZeroPageNZ()
{
  uint8_t data = loadZeroPage();
  updateNZ(data);
  return data;
}

void Mos6502::updateNZ(uint8_t value)
{
  zero_ = (value == 0);
  negative_ = (value & 0x80) != 0;
}

uint8_t Mos6502::transfer(uint8_t value)
{
  updateNZ(value);
  return value;
}

void Mos6502::compareFlags(uint8_t value1, uint8_t value2)
{
  // Carry flag is like an active low borrow
  // https://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
  uint16_t sum = value1 + ~value2 + 1;
  bool carry6 = ((value1 & 0x7f) + (~value2 & 0x7F) + 1) & 0x80;
  carry_ = !(sum & 0x100);
  overflow_ = carry_ != carry6;
  sum &= 0xFF;
  updateNZ(sum);
}

uint8_t Mos6502::getStatus() const
{
  uint8_t status =
    ((negative_ ? 1 : 0) << 7) |
    ((overflow_ ? 1 : 0) << 6) |
    (1 << 5) |
    ((brk_ ? 1 : 0) << 4) |
    ((decimal_mode_ ? 1 : 0) << 3) |
    ((irq_disable_ ? 1 : 0) << 2) |
    ((zero_ ? 1 : 0) << 1) |
    (carry_ ? 1 : 0);
  return status;
}

unsigned Mos6502::execOne()
{
  // Right after reset, the processor will read memory addresses 0xFFFC and 0xFFFD into PC
  if (reseting_)
  {
    reseting_ = false;
    // TODO which order are value read, probably doesn't matter
    uint8_t pc_lo = read_callback_(0xFFFC);
    uint8_t pc_hi = read_callback_(0xFFFD);
    pc_ = (pc_hi << 8) | pc_lo;
    return 2;  // assume 2 instructions to read ROM into PC
  }

  uint8_t op_code = read_callback_(pc_);
  instr_[0] = op_code;
  const OpInfo& op_info = op_table_[op_code];
  for (unsigned ii = 1; ii < op_info.len; ++ii)
  {
    instr_[ii] = read_callback_(pc_ + ii);
  }
  instr_len_ = op_info.len;
  pc_ += op_info.len;
  try {
    unsigned cycle_count = op_info.func(*this);
    instr_cycle_count_ += cycle_count;
    return cycle_count;
  }
  catch (const std::exception& ex)
  {
    std::ostringstream ss;
    ss << "Error running instruction at PC=" << std::hex << (pc_ - op_info.len) << " : " << ex.what();
    throw std::runtime_error(ss.str());
  }
}

const char* Mos6502::getOpName(uint8_t opcode) const
{
  return op_table_.at(opcode).name;
}

void Mos6502::outputRegs(std::ostream& os) const
{
  os << "  PC: " << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(pc_) << std::endl;
  os << "  A: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(a_) << std::endl;
  os << "  X: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(x_) << std::endl;
  os << "  Y: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(y_) << std::endl;
}

unsigned Mos6502::branch()
{
  uint16_t old_pc = pc_;
  pc_ += static_cast<int8_t>(instr_[1]);

  // same page
  if ((pc_ & 0xff00) == (old_pc & 0xff00))
  {
    return 3;
  }
  //next page
  return 4; //cycles
}


void Mos6502::addArithmeticInstructions()
{
  // https://www.masswerk.at/6502/6502_instruction_set.html

  auto inc_op = [](Mos6502& cpu, uint8_t operand) -> uint8_t
  {
    ++operand;
    cpu.updateNZ(operand);
    return operand;
  };

  addInstructionUnaryZeroPage(0xE6, "INC zpg", inc_op);
  addInstructionUnaryZeroPageX(0xF6, "INC zpg,x", inc_op);
  addInstructionUnaryAbsolute(0xEE, "INC abs", inc_op);
  addInstructionUnaryAbsoluteX(0xFE, "INC abs,x", inc_op);

  // increment X by 1
  addInstruction(0xE8, "INX", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.updateNZ(++cpu.x_);
    return 2; //cycles
  });

  // increment Y by 1
  addInstruction(0xC8, "INY", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.updateNZ(++cpu.y_);
    return 2; //cycles
  });

  // decrement at zeropage Memory by 1
  addInstruction(0xC6, "DEC zpg", 2, [](Mos6502& cpu) -> unsigned
  {
    uint16_t addr = cpu.instr_[1];
    uint8_t data = cpu.read_callback_(addr);
    --data;
    cpu.updateNZ(data);
    cpu.write_callback_(addr, data);
    return 5; //cycles
  });

  // decrement X by 1
  addInstruction(0xCA, "DEX", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.updateNZ(--cpu.x_);
    return 2; //cycles
  });

  // decrement Y by 1
  addInstruction(0x88, "DEY", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.updateNZ(--cpu.y_);
    return 2; //cycles
  });

  // add with carry
  auto adc_op = [](Mos6502& cpu, uint8_t operand)
  {
    uint16_t sum = cpu.a_ + operand + (cpu.carry_ ? 1 : 0);
    bool carry6 = ((cpu.a_ & 0x7f) + (operand & 0x7F) + (cpu.carry_ ? 1 : 0)) & 0x80;
    cpu.a_ = sum;
    cpu.carry_ = (sum & 0x100);
    cpu.overflow_ = cpu.carry_ != carry6;
    cpu.updateNZ(cpu.a_);
  };
  addInstructionImmediate(0x69, "ADC #", adc_op);
  addInstructionZeroPage(0x65, "ADC zpg", adc_op);
  addInstructionZeroPageX(0x75, "ADC zpg", adc_op);
  addInstructionAbsolute(0x6D, "ADC abs", adc_op);
  addInstructionAbsoluteX(0x7D, "ADC abs,x", adc_op);
  addInstructionAbsoluteY(0x79, "ADC abs,y", adc_op);
  addInstructionIndirectX(0x61, "ADC (indirect),x", adc_op);
  addInstructionIndirectY(0x71, "ADC (indirect,y)", adc_op);

  /*
  subtract with carry
  A - M - C̅ -> A
  N	Z	C	I	D	V
  +	+	+	-	-	+
  addressing	assembler	opc	bytes	cycles
  immediate	SBC #oper	E9	2	2
  zeropage	SBC oper	E5	2	3
  zeropage,X	SBC oper,X	F5	2	4
  absolute	SBC oper	ED	3	4
  absolute,X	SBC oper,X	FD	3	4*
  absolute,Y	SBC oper,Y	F9	3	4*
  (indirect,X)	SBC (oper,X)	E1	2	6
  (indirect),Y	SBC (oper),Y	F1	2	5*
  */
  auto sbc_op = [](Mos6502& cpu, uint8_t operand)
  {
    uint16_t sum = cpu.a_ + ~operand + (cpu.carry_ ? 1 : 0);
    bool carry6 = ((cpu.a_ & 0x7f) + (~operand & 0x7F) + (cpu.carry_ ? 1 : 0)) & 0x80;
    cpu.a_ = sum;
    cpu.carry_ = !(sum & 0x100);
    cpu.overflow_ = cpu.carry_ != carry6;
    cpu.updateNZ(cpu.a_);
  };
  addInstructionImmediate(0xE9, "SBC #", sbc_op);
  addInstructionZeroPage(0xE5, "SBC zpg", sbc_op);
  addInstructionZeroPageX(0xF5, "SBC zpg", sbc_op);
  addInstructionAbsolute(0xED, "SBC abs", sbc_op);
  addInstructionAbsoluteX(0xFD, "SBC abs,x", sbc_op);
  addInstructionAbsoluteY(0xF9, "SBC abs,y", sbc_op);
  addInstructionIndirectX(0xE1, "SBC (indirect),x", sbc_op);
  addInstructionIndirectY(0xF1, "SBC (indirect,y)", sbc_op);
}

void Mos6502::addLoadInstructions()
{

  // Load A
  auto op_lda = [](Mos6502& cpu, uint8_t data)
  {
    cpu.updateNZ(data);
    cpu.a_ = data;
  };

  addInstructionImmediate(0xA9, "LDA #", op_lda);
  addInstructionZeroPage(0xA5, "LDA zpg", op_lda);
  addInstructionZeroPageX(0xB5, "LDA zpg,x", op_lda);
  addInstructionAbsolute(0xAD, "LDA abs", op_lda);
  addInstructionAbsoluteX(0xBD, "LDA abs,x", op_lda);
  addInstructionAbsoluteY(0xB9, "LDA abs,y", op_lda);
  addInstructionIndirectY(0xB1, "LDA (indirect),y", op_lda);

  // Load X
  auto op_ldx = [](Mos6502& cpu, uint8_t data)
  {
    cpu.updateNZ(data);
    cpu.x_ = data;
  };

  addInstructionImmediate(0xA2, "LDX #", op_ldx);
  addInstructionZeroPage(0xA6, "LDX zpg", op_ldx);
  addInstructionAbsolute(0xAE, "LDX abs", op_ldx);
  addInstructionAbsoluteY(0xBE, "LDX abs,y", op_ldx);

  // Load Y
  auto op_ldy = [](Mos6502& cpu, uint8_t data)
  {
    cpu.updateNZ(data);
    cpu.y_ = data;
  };

  addInstructionImmediate(0xA0, "LDY #", op_ldy);
  addInstructionZeroPage(0xA4, "LDY zpg", op_ldy);
  addInstructionZeroPageX(0xB4, "LDY zpg,x", op_ldy);
  addInstructionAbsolute(0xAC, "LDY abs", op_ldy);
  addInstructionAbsoluteX(0xBC, "LDY abs,x", op_ldy);
}

void Mos6502::addStoreInstructions()
{
  // STA store accumulator into memory zpg,X
  addInstruction(0x95, "STA zpg,x", 2, [](Mos6502& cpu) -> unsigned
  {
    uint16_t addr = (cpu.instr_[1] + cpu.x_) & 0xFF;
    cpu.write_callback_(addr, cpu.a_);
    return 4; //cycles
  });

  // STA zeropage
  addInstruction(0x85, "STA zpg", 2, [](Mos6502& cpu) -> unsigned
  {
    cpu.write_callback_(cpu.instr_[1], cpu.a_);
    return 3; //cycles
  });

  // STA abs
  addInstruction(0x8D, "STA abs", 3, [](Mos6502& cpu) -> unsigned
  {
    uint16_t addr = cpu.getAbsoluteAddress();
    cpu.write_callback_(addr, cpu.a_);
    return 4; //cycles
  });

  // STA abs,x
  addInstruction(0x9D, "STA abs,x", 3, [](Mos6502& cpu) -> unsigned
  {
    uint16_t addr = cpu.getAbsoluteAddress() + cpu.x_;
    cpu.write_callback_(addr, cpu.a_);
    return 5; //cycles
  });

  // STA abs,y
  addInstruction(0x99, "STA abs,y", 3, [](Mos6502& cpu) -> unsigned
  {
    uint16_t addr = cpu.getAbsoluteAddress() + cpu.y_;
    cpu.write_callback_(addr, cpu.a_);
    return 5; //cycles
  });

  // STX zeropage
  addInstruction(0x86, "STX zpg", 2, [](Mos6502& cpu) -> unsigned
  {
    cpu.write_callback_(cpu.instr_[1], cpu.x_);
    return 3; //cycles
  });

  // STY zeropage
  addInstruction(0x84, "STY zpg", 2, [](Mos6502& cpu) -> unsigned
  {
    cpu.write_callback_(cpu.instr_[1], cpu.y_);
    return 3; //cycles
  });

  // STY zeropage,x
  addInstruction(0x94, "STY zpg,x", 2, [](Mos6502& cpu) -> unsigned
  {
    uint16_t addr = (cpu.instr_[1] + cpu.x_) & 0xFF;
    cpu.write_callback_(addr, cpu.y_);
    return 4; //cycles
  });

  // STY absolute
  addInstruction(0x8C, "STY abs", 3, [](Mos6502& cpu) -> unsigned
  {
    cpu.write_callback_(cpu.getAbsoluteAddress(), cpu.y_);
    return 4; //cycles
  });
}

void Mos6502::addTransferInstructions()
{
  // TXS move X to SP
  addInstruction(0x9A, "TXS", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.sp_ = cpu.transfer(cpu.x_);
    return 2; //cycles
  });

  // TSX move SP to X
  addInstruction(0xBA, "TSX", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.x_ = cpu.transfer(cpu.sp_);
    return 2; //cycles
  });

  // transfer x to a
  addInstruction(0x8A, "TXA", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.a_ = cpu.transfer(cpu.x_);
    return 2; //cycles
  });

  // transfer A to X
  addInstruction(0xAA, "TAX", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.x_ = cpu.transfer(cpu.a_);
    return 2; //cycles
  });

  // transfer A to Y
  addInstruction(0xA8, "TAY", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.y_ = cpu.transfer(cpu.a_);
    return 2; //cycles
  });

  // transfer Y to A
  addInstruction(0x98, "TYA", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.a_ = cpu.transfer(cpu.y_);
    return 2; //cycles
  });
}

void Mos6502::addSpecialInstructions()
{
  // NOP (no operation)
  addInstruction(0xEA, "NOP", 1, [](Mos6502& cpu) -> unsigned
  {
    return 2;
  });

  // SEI set interupt disable
  addInstruction(0x78, "SEI", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.irq_disable_ = true;
    return 2;
  });

  // CLD clear decimal mode
  addInstruction(0xD8, "CLD", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.decimal_mode_ = false;
    return 2;
  });

  // SEC set carry flag
  addInstruction(0x38, "SEC", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.carry_ = true;
    return 2;
  });

  // clear carry flag
  addInstruction(0x18, "CLC", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.carry_ = false;
    return 2;
  });

}


void Mos6502::addBranchInstructions()
{
  // BNE Branching if not equal to zero
  addInstruction(0xD0, "BNE", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch if not zero (not equal)
    if (!cpu.zero_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BEQ Branching if equal to zero
  addInstruction(0xF0, "BEQ", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch zero (equal)
    if (cpu.zero_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BPL Branching if plus
  addInstruction(0x10, "BPL", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch on N = 0
    if (!cpu.negative_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BMI Branching if minus
  addInstruction(0x30, "BMI", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch on N = 1
    if (cpu.negative_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BCS Branching if carry set
  addInstruction(0xB0, "BCS", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch on c = 1
    if (cpu.carry_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BCC Branching if carry set
  addInstruction(0x90, "BCC", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch on c = 0
    if (!cpu.carry_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BVC Branching overflow clear
  addInstruction(0x50, "BVC", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch on V = 0
    if (!cpu.overflow_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // BVS Branching overflow set
  addInstruction(0x70, "BVS", 2, [](Mos6502& cpu) -> unsigned
  {
    // branch on V = 1
    if (cpu.overflow_)
    {
      return cpu.branch();
    }
    return 2;
  });

  // JSR Jump to New Location Saving Return Address
  addInstruction(0x20, "JSR", 3, [](Mos6502& cpu) -> unsigned
  {
    uint16_t stack_addr = 0x100 + cpu.sp_;
    // PC is incremented by +3 before this function is called
    // however 6502 will store PC+2 to stack (not PC+3) which would be next instruction
    uint16_t ret_addr = cpu.pc_ - 1;
    cpu.write_callback_(stack_addr, ret_addr >> 8);
    cpu.write_callback_(stack_addr - 1, ret_addr & 0xff );

    cpu.sp_ -= 2;
    cpu.pc_ =  (cpu.instr_[2] << 8  ) + cpu.instr_[1] ;
    return 6; //cycles
  });

  // Return from subroutine
  addInstruction(0x60, "RTS", 1, [](Mos6502& cpu) -> unsigned
  {
    uint16_t stack_addr = 0x100 + cpu.sp_;
    uint16_t ret_addr = cpu.pc_ + 2;
    uint8_t pcl = cpu.read_callback_(stack_addr + 1);
    uint8_t pch = cpu.read_callback_(stack_addr + 2);
    cpu.sp_ += 2;
    // return to address on stack +1
    cpu.pc_ = ((pch << 8) | pcl) + 1;
    return 6; //cycles
  });


  // jmp absolute
  addInstruction(0x4C, "JMP", 3, [](Mos6502& cpu) -> unsigned
  {
    cpu.pc_ = (cpu.instr_[2] << 8) | cpu.instr_[1];
    return 3; //cycles
  });
}


void Mos6502::addStackInstructions()
{
  // Push accumulator onto stack
  addInstruction(0x48, "PHA", 1, [](Mos6502& cpu) -> unsigned
  {
    uint16_t write_addr = 0x100 + cpu.sp_;
    cpu.write_callback_(write_addr, cpu.a_);
    cpu.sp_ -= 1;
    return 3; //cycles
  });

  // Pull accumulator from stack
  addInstruction(0x68, "PLA", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.sp_ += 1;
    uint16_t read_addr = 0x100 + cpu.sp_;
    cpu.a_ = cpu.read_callback_(read_addr);
    cpu.updateNZ(cpu.a_);
    return 3; //cycles
  });
}


void Mos6502::addCompareInstructions()
{
  // compare A
  auto cmp_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.compareFlags(cpu.a_, operand);
  };

  /*
  addressing	assembler	opc	bytes	cycles
  immediate	CMP #oper	C9	2	2
  zeropage	CMP oper	C5	2	3
  zeropage,X	CMP oper,X	D5	2	4
  absolute	CMP oper	CD	3	4
  absolute,X	CMP oper,X	DD	3	4*
  absolute,Y	CMP oper,Y	D9	3	4*
  (indirect,X)	CMP (oper,X)	C1	2	6
  (indirect),Y	CMP (oper),Y	D1	2	5*
  */
  addInstructionImmediate(0xC9, "CMP #", cmp_op);
  addInstructionZeroPage(0xC5, "CMP zpg", cmp_op);
  addInstructionZeroPageX(0xD5, "CMP zpg,x", cmp_op);
  addInstructionAbsolute(0xCD, "CMP abs", cmp_op);
  addInstructionAbsoluteX(0xDD, "CMP abs,x", cmp_op);
  addInstructionAbsoluteY(0xD9, "CMP abs,y", cmp_op);
  addInstructionIndirectX(0xC1, "CMP (indirect,x)", cmp_op);
  addInstructionIndirectY(0xD1, "CMP (indirect),y", cmp_op);

  /*
  Compare Memory and Index X
  X - M
  N	Z	C	I	D	V
  +	+	+	-	-	-
  addressing	assembler	opc	bytes	cycles
  immediate	CPX #oper	E0	2	2
  zeropage	CPX oper	E4	2	3
  absolute	CPX oper	EC	3	4
  */
  auto cpx_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.compareFlags(cpu.x_, operand);
  };
  addInstructionImmediate(0xE0, "CPX #", cpx_op);
  addInstructionZeroPage(0xE4, "CPX zpg", cpx_op);
  addInstructionAbsolute(0xEC, "CPX abs", cpx_op);

  /*
    Compare Memory and Index Y
    Y - M
    N	Z	C	I	D	V
    +	+	+	-	-	-
    addressing	assembler	opc	bytes	cycles
    immediate	CPY #oper	C0	2	2
    zeropage	CPY oper	C4	2	3
    absolute	CPY oper	CC	3	4
  */
  auto cpy_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.compareFlags(cpu.y_, operand);
  };
  addInstructionImmediate(0xC0, "CPY #", cpy_op);
  addInstructionZeroPage(0xC4, "CPY zpg", cpy_op);
  addInstructionAbsolute(0xCC, "CPY abs", cpy_op);
}

void Mos6502::addShiftAndRotateInstructions()
{
  /*
  Arithmetic Shift Left
  C <- [76543210] <- 0
  N	Z	C	I	D	V
  +	+	+	-	-	-
  addressing	assembler	opc	bytes	cycles
  accumulator	ASL A	0A	1	2
  zeropage	ASL oper	06	2	5
  zeropage,X	ASL oper,X	16	2	6
  absolute	ASL oper	0E	3	6
  absolute,X	ASL oper,X	1E	3	7
  */
  auto asl_op = [](Mos6502& cpu, uint8_t operand) -> uint8_t
  {
    cpu.carry_ = operand & 7;
    operand <<= 1;
    return operand;
  };
  addInstructionUnaryA(0x0A, "ASL A", asl_op);
  addInstructionUnaryZeroPage(0x06, "ASL zpg", asl_op);
  addInstructionUnaryZeroPageX(0x16, "ASL zpg,x", asl_op);
  addInstructionUnaryAbsolute(0x0E, "ASL abs", asl_op);
  addInstructionUnaryAbsoluteX(0x1E, "ASL abs,x", asl_op);

  // arithmatic shift right accumulator
  addInstruction(0x4A, "LSR", 1, [](Mos6502& cpu) -> unsigned
  {
    cpu.carry_ = cpu.a_ & 1;
    cpu.a_ >>= 1;
    return 2; //cycles
  });

  /*
  Rotate One Bit Right (Memory or Accumulator)
  C -> [76543210] -> C
  N	Z	C	I	D	V
  +	+	+	-	-	-
  addressing	assembler	opc	bytes	cycles
  accumulator	ROR A	6A	1	2
  zeropage	ROR oper	66	2	5
  zeropage,X	ROR oper,X	76	2	6
  absolute	ROR oper	6E	3	6
  absolute,X	ROR oper,X	7E	3	7
  */
  auto ror_op = [](Mos6502& cpu, uint8_t operand) -> uint8_t
  {
    bool carry_out = operand & 1;
    operand >>= 1;
    operand |= cpu.carry_ ? 0x80 : 0;
    cpu.carry_ = carry_out;
    cpu.updateNZ(operand);
    return operand;
  };
  addInstructionUnaryA(0x6A, "ROR A", ror_op);
  addInstructionUnaryZeroPage(0x66, "ROR zpg", ror_op);
  addInstructionUnaryZeroPageX(0x76, "ROR zpg,x", ror_op);
  addInstructionUnaryAbsolute(0x6E, "ROR abs", ror_op);
  addInstructionUnaryAbsoluteX(0x7E, "ROR abs,x", ror_op);

  /*
  C <- [76543210] <- C
  N	Z	C	I	D	V
  +	+	+	-	-	-
  addressing	assembler	opc	bytes	cycles
  accumulator	ROL A	2A	1	2
  zeropage	ROL oper	26	2	5
  zeropage,X	ROL oper,X	36	2	6
  absolute	ROL oper	2E	3	6
  absolute,X	ROL oper,X	3E	3	7
  */
  auto rol_op = [](Mos6502& cpu, uint8_t operand) -> uint8_t
  {
    bool carry_out = operand & 0x80;
    operand <<= 1;
    operand |= cpu.carry_ ? 1 : 0;
    cpu.carry_ = carry_out;
    cpu.updateNZ(operand);
    return operand;
  };
  addInstructionUnaryA(0x2A, "ROL A", rol_op);
  addInstructionUnaryZeroPage(0x26, "ROL zpg", rol_op);
  addInstructionUnaryZeroPageX(0x36, "ROL zpg,x", rol_op);
  addInstructionUnaryAbsolute(0x2E, "ROL abs", rol_op);
  addInstructionUnaryAbsoluteX(0x3E, "ROL abs,x", rol_op);
}


void Mos6502::addLogicalInstructions()
{
  // Logical And
  /*
  addressing	assembler	opc	bytes	cycles
  immediate	AND #oper	29	2	2
  zeropage	AND oper	25	2	3
  zeropage,X	AND oper,X	35	2	4
  absolute	AND oper	2D	3	4
  absolute,X	AND oper,X	3D	3	4*
  absolute,Y	AND oper,Y	39	3	4*
  (indirect,X)	AND (oper,X)	21	2	6
  (indirect),Y	AND (oper),Y	31	2	5*
  */
  auto and_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.a_ &= operand;
    cpu.updateNZ(cpu.a_);
  };
  addInstructionImmediate(0x29, "AND #", and_op);
  addInstructionZeroPage(0x25, "AND zpg", and_op);
  addInstructionZeroPageX(0x35, "AND zpg,x", and_op);
  addInstructionAbsolute(0x2D, "AND abs", and_op);
  addInstructionAbsoluteX(0x3D, "AND abs,x", and_op);
  addInstructionAbsoluteX(0x39, "AND abs,y", and_op);
  addInstructionIndirectX(0x21, "AND (indirect),x", and_op);
  addInstructionIndirectY(0x31, "AND (indirect,y)", and_op);

  // Logical Or
  auto or_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.a_ |= operand;
    cpu.updateNZ(cpu.a_);
  };
  addInstructionImmediate(0x09, "ORA #", or_op);
  addInstructionZeroPage(0x05, "ORA zpg", or_op);

  // Exclusive or
  auto eor_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.a_ ^= operand;
    cpu.updateNZ(cpu.a_);
  };
  addInstructionImmediate(0x49, "EOR #", eor_op);
  addInstructionZeroPage(0x45, "EOR zpg", eor_op);

  /*
  A AND M -> Z, M7 -> N, M6 -> V
  N	Z	C	I	D	V
  M7	+	-	-	-	M6
  addressing	assembler	opc	bytes	cycles
  zeropage	BIT oper	24	2	3
  absolute	BIT oper	2C	3	4
  */
  auto bit_op = [](Mos6502& cpu, uint8_t operand)
  {
    cpu.zero_ = cpu.a_ & operand;
    cpu.negative_= operand & 0x80;
    cpu.overflow_ = operand & 0x40;
  };
  addInstructionZeroPage(0x24, "BIT zpg", bit_op);
  addInstructionAbsolute(0x2C, "BIT abs", bit_op);

}