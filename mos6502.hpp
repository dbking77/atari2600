#ifndef ATARI2600_MOS6502_HPP_GUARD
#define ATARI2600_MOS6502_HPP_GUARD

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>

class Mos6502
{
public:
  // ReadCallback takes a 16bit address and returns an 8bit value
  using ReadCallback = std::function<uint8_t(uint16_t)>;

  // WriteCallback takes a 16bit address and 8bit value
  using WriteCallback = std::function<void(uint16_t, uint8_t)>;

  Mos6502(ReadCallback read_callback, WriteCallback write_callback);

  // Output register values to stream
  void outputRegs(std::ostream& os) const;

  uint8_t a_ = 0;
  uint8_t x_ = 0;
  uint8_t y_ = 0;
  uint8_t sp_ = 0;
  uint16_t pc_ = 0;

  // Used during instruction decoding
  std::array<uint8_t, 3> instr_ = {0,0,0};
  uint8_t instr_len_ = 0;

  bool carry_ = false;
  bool zero_ = false;
  bool irq_disable_ = true;
  bool decimal_mode_ = false;
  bool brk_ = true;
  bool overflow_ = false;
  bool negative_ = false;

  bool reseting_ = true;

  unsigned instr_cycle_count_ = 0;

  uint8_t getStatus() const;

  /**
   * Execute one instruction, return number of instruction clock cycles needed
   */
  unsigned execOne();

  const char* getOpName(uint8_t opcode) const;

protected:

  // These must be provided
  ReadCallback read_callback_ = nullptr;
  WriteCallback write_callback_ = nullptr;

  //https://www.masswerk.at/6502/6502_instruction_set.html

  // Takea a pointer to Mos6502 instruction and updates processor state
  // Returns number of instruction cycles need to complete instruction
  using OpFunc = unsigned(*)(Mos6502& cpu);

  struct OpInfo
  {
    const char* name;
    OpFunc func;
    // len of opcode 1,2,3 bytes
    uint8_t len;
  };

  std::array<OpInfo, 256> op_table_;

  /**
   * @brief add instruction to op_table, and description to decode table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param op_len len of opcode 1, 2, or 3 bytes
   * @param op_func pointer to unction to call to execute instruction
  */
  void addInstruction(uint8_t opcode, const char* op_name, uint8_t op_len, OpFunc op_func);

  /**
   * @brief add immediate instruction to op_table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   * For this the operand value passed to labmda will always be immediate value from instruction
   */
  template<unsigned CYCLES=2, typename OP_FUNC_TYPE>
  void addInstructionImmediate(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      // Stateless lamdas don't have a default constructor, so use this hack to allow use
      // of operator() from stateless lambda
      // https://stackoverflow.com/questions/64178997/stateless-lambdas-as-static-local-variablef
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      op_func(cpu, cpu.instr_[1]) ;
      return CYCLES;
    });
  }

  /**
   * @brief add immediate instruction to op_table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   * For this the operand value passed to labmda will always be zeropage load based on address from instruction
   */
  template<unsigned CYCLES=3, typename OP_FUNC_TYPE>
  void addInstructionZeroPage(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      op_func(cpu, cpu.loadZeroPage());
      return CYCLES;
    });
  }

  /**
   * @brief add immediate instruction to op_table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   */
  template<typename OP_FUNC_TYPE>
  void addInstructionZeroPageX(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr = (cpu.instr_[1] + cpu.x_) & 0xFF;
      uint8_t data = cpu.read_callback_(addr);
      op_func(cpu, data);
      return 4;
    });
  }


  /**
   * @brief add absolute instruction to op_table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   */
  template<typename OP_FUNC_TYPE>
  void addInstructionAbsolute(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 3, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr = cpu.getAbsoluteAddress();
      uint8_t data = cpu.read_callback_(addr);
      op_func(cpu, data);
      return 4;
    });
  }

  /**
   * @brief add absolute,X instruction to op_table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   */
  template<typename OP_FUNC_TYPE>
  void addInstructionAbsoluteX(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 3, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t base_addr = cpu.getAbsoluteAddress();
      uint16_t addr = base_addr + cpu.x_;
      uint8_t data = cpu.read_callback_(addr);
      op_func(cpu, data);
      // +1 extra cycle if high byte of address changed
      return 4 + hasHighByteChanged(base_addr, addr) ? 1 : 0;
    });
  }

  /**
   * @brief add absolute,Y instruction to op_table
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   */
  template<typename OP_FUNC_TYPE>
  void addInstructionAbsoluteY(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 3, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t base_addr = cpu.getAbsoluteAddress();
      uint16_t addr = base_addr + cpu.y_;
      uint8_t data = cpu.read_callback_(addr);
      op_func(cpu, data);
      // +1 extra cycle if high byte of address changed
      return 4 + hasHighByteChanged(base_addr, addr) ? 1 : 0;
    });
  }

  /**
   * @brief add indirect,X instruction to op_table (indirect-indexed)
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   * addr = (indirect),y
   * first load 16bit indirect address from memory using instruction opcode as zeropage address
   * then add indirect address to y
   */
  template<typename OP_FUNC_TYPE>
  void addInstructionIndirectX(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr_zpg = cpu.instr_[1] + cpu.x_;
      uint8_t addr_lo = cpu.read_callback_(addr_zpg & 0xFF);
      uint8_t addr_hi = cpu.read_callback_((addr_zpg + 1) & 0xFF); // todo does this wrap
      uint16_t addr = ((addr_hi << 8) | addr_lo);
      uint8_t data = cpu.read_callback_(addr);
      op_func(cpu, data);
      return 6;
    });
  }

  /**
   * @brief add indirect,Y instruction to op_table (indirect-indexed)
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand
   *
   * addr = (indirect),y
   * first load 16bit indirect address from memory using instruction opcode as zeropage address
   * then add indirect address to y
   */
  template<typename OP_FUNC_TYPE>
  void addInstructionIndirectY(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr_zpg = cpu.instr_[1];
      uint8_t addr_lo = cpu.read_callback_(addr_zpg);
      uint8_t addr_hi = cpu.read_callback_((addr_zpg + 1) & 0xFF); // todo does this wrap
      uint16_t addr = ((addr_hi << 8) | addr_lo) + cpu.y_;
      uint8_t data = cpu.read_callback_(addr);
      op_func(cpu, data);
      return 6;  // 5* for cmp?
    });
  }

  /**
   * @brief add a unary operation that modifies accumulator
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand, and returns an 8bit value
   */
  template<unsigned CYCLES=2, typename OP_FUNC_TYPE>
  void addInstructionUnaryA(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 1, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      cpu.a_ = op_func(cpu, cpu.a_) ;
      return CYCLES;
    });
  }

  /**
   * @brief add a unary operation that modifies a zeropage address
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand, and returns an 8bit value
   */
  template<unsigned CYCLES=5, typename OP_FUNC_TYPE>
  void addInstructionUnaryZeroPage(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr = cpu.instr_[1];
      cpu.write_callback_(addr, op_func(cpu, cpu.read_callback_(addr)));
      return CYCLES;
    });
  }

  /**
   * @brief add a unary operation that modifies a zeropage + x address
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand, and returns an 8bit value
   */
  template<unsigned CYCLES=6, typename OP_FUNC_TYPE>
  void addInstructionUnaryZeroPageX(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 2, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr = (cpu.instr_[1] + cpu.x_) & 0xFF;
      cpu.write_callback_(addr, op_func(cpu, cpu.read_callback_(addr)));
      return CYCLES;
    });
  }

  /**
   * @brief add a unary operation that modifies an absolute address
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand, and returns an 8bit value
   */
  template<unsigned CYCLES=6, typename OP_FUNC_TYPE>
  void addInstructionUnaryAbsolute(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 3, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr = cpu.getAbsoluteAddress();
      cpu.write_callback_(addr, op_func(cpu, cpu.read_callback_(addr)));
      return CYCLES;
    });
  }

  /**
   * @brief add a unary operation that modifies an absolute,x address
   * @param opcode 8bit opcode for instruction
   * @param op_name short description name for opcode (ADC, AND, ASL, ...)
   * @param unused_op_func stateless lambda operation that takes two inputs, a Mos6502 reference, and a 8bit operand, and returns an 8bit value
   */
  template<unsigned CYCLES=7, typename OP_FUNC_TYPE>
  void addInstructionUnaryAbsoluteX(uint8_t opcode, const char* op_name, OP_FUNC_TYPE& unused_op_func)
  {
    addInstruction(opcode, op_name, 3, [](Mos6502& cpu) -> unsigned
    {
      OP_FUNC_TYPE* op_func_pointer = nullptr;
      OP_FUNC_TYPE op_func = *op_func_pointer;
      uint16_t addr = cpu.getAbsoluteAddress() + cpu.x_;
      cpu.write_callback_(addr, op_func(cpu, cpu.read_callback_(addr)));
      return CYCLES;
    });
  }

  inline uint16_t getAbsoluteAddress() const
  {
    uint16_t addr = (instr_[2] << 8) | instr_[1];
    return addr;
  }

  static inline bool hasHighByteChanged(uint16_t addr1, uint16_t addr2)
  {
    return (addr1 ^ addr2) & 0xFF00;
  }

  /**
   * @brief update N (negative) and Z (zero) flags with value
  */
  void updateNZ(uint8_t value);

  /**
   * @brief update N (negative) and Z (zero) flags with value
   * @return input value
   */
  uint8_t transfer(uint8_t value);

  /**
   * @brief load immediate value at PC+1 and update NZ flags
  */
  uint8_t loadImmediateNZ();

  /**
   * @brief load value from zeropage and update NZ flags
  */
  uint8_t loadZeroPageNZ();

  /**
   * @brief load value from zeropage
  */
  uint8_t loadZeroPage();

  /**
   * @brief compare 2 value (value1 - value2) and set N, Z, C flags
   */
  void compareFlags(uint8_t value1, uint8_t value2);

  /**
   * @brief branch
   */
  unsigned branch();

  // Helpers
  void addArithmeticInstructions();
  void addLoadInstructions();
  void addStoreInstructions();
  void addTransferInstructions();
  void addSpecialInstructions();
  void addBranchInstructions();
  void addStackInstructions();
  void addCompareInstructions();
  void aaddShiftAndRotateInstructions();
  void addShiftAndRotateInstructions();
  void addLogicalInstructions();
};

#endif  // ATARI2600_MOS6502_HPP_GUARD
