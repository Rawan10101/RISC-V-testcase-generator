# RISC-V Testcase Generator

This project is a **C++ program** that generates random test cases for the **RISC-V instruction set**. It produces instructions in **binary, assembly, and hexadecimal formats**.

---

## Features

- Supports all major **RISC-V instruction types**:
  - **R-Type:** `ADD`, `SUB`, `SLL`, `SLT`, `SLTU`, `XOR`, `SRL`, `SRA`, `OR`, `AND`
  - **I-Type:** `ADDI`, `SLTI`, `SLTIU`, `XORI`, `ORI`, `ANDI`, `SLLI`, `SRLI`, `SRAI`, loads (`LB`, `LH`, `LW`, `LBU`, `LHU`)
  - **S-Type:** `SB`, `SH`, `SW`
  - **SB-Type (Branch):** `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`
  - **U-Type:** `LUI`, `AUIPC`
  - **UJ-Type:** `JAL`
- Generates **randomized instructions** with:
  - User-defined number of test cases
  - User-defined number of instructions per test case
  - User-defined number of registers to use
- Automatically initializes used registers with `ADDI` instructions
- Produces output in **three formats**:
  1. **Binary** (`binaryN.txt`)
  2. **Assembly** (`assemblyN.s`)
  3. **Hexadecimal** (`hexN.v`)

---

## Getting Started

### Requirements

- **C++17** or later
- A compiler supporting standard libraries: `iostream`, `unordered_map`, `unordered_set`, `vector`, `fstream`, etc.

### Usage

1. Compile the program:

```bash
g++ riscv_generator.cpp -o riscv_gen.exe
