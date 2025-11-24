#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <set>

using namespace std;

static const unordered_map<string,string> OPCODES = {
    {"LUI","0110111"},{"AUIPC","0010111"},{"JAL","1101111"},{"JALR","1100111"},
    {"BEQ","1100011"},{"BNE","1100011"},{"BLT","1100011"},{"BGE","1100011"},{"BLTU","1100011"},{"BGEU","1100011"},
    {"LB","0000011"},{"LH","0000011"},{"LW","0000011"},{"LBU","0000011"},{"LHU","0000011"},
    {"SB","0100011"},{"SH","0100011"},{"SW","0100011"},
    {"ADDI","0010011"},{"SLTI","0010011"},{"SLTIU","0010011"},{"XORI","0010011"},{"ORI","0010011"},{"ANDI","0010011"},{"SLLI","0010011"},{"SRLI","0010011"},{"SRAI","0010011"},
    {"ADD","0110011"},{"SUB","0110011"},{"SLL","0110011"},{"SLT","0110011"},{"SLTU","0110011"},{"XOR","0110011"},{"SRL","0110011"},{"SRA","0110011"},{"OR","0110011"},{"AND","0110011"}
};

static const unordered_map<string,string> FUNCT_CODES = {
    {"JALR","000"}, {"BEQ","000"}, {"BNE","001"}, {"BLT","100"}, {"BGE","101"}, {"BLTU","110"}, {"BGEU","111"},
    {"LB","000"}, {"LH","001"}, {"LW","010"}, {"LBU","100"}, {"LHU","101"},
    {"SB","000"}, {"SH","001"}, {"SW","010"},
    {"ADDI","000"}, {"SLTI","010"}, {"SLTIU","011"}, {"XORI","100"}, {"ORI","110"}, {"ANDI","111"},
    {"SLLI","001"}, {"SRLI","101"}, {"SRAI","101"},
    {"ADD","000"}, {"SUB","000"}, {"SLL","001"}, {"SLT","010"}, {"SLTU","011"}, {"XOR","100"}, {"SRL","101"}, {"SRA","101"}, {"OR","110"}, {"AND","111"}
};

static const unordered_set<string> LOAD_INSTRUCTION_NAMES = {"LB","LH","LW","LBU","LHU"};
static const unordered_set<string> STORE_INSTRUCTION_NAMES = {"SB","SH","SW"};
static const unordered_set<string> SHIFT_IMMEDIATE_INSTRUCTION_NAMES = {"SLLI","SRLI","SRAI"};

// Types -> instruction sets
static const unordered_map<string, vector<string>> TYPES_TO_INSTRUCTION = {
    {"U_TYPE", {"LUI","AUIPC"}},
    {"UJ_TYPE", {"JAL"}},
    {"SB_TYPE", {"BEQ","BNE","BLT","BGE","BLTU","BGEU"}},
    {"I_TYPE", {"JALR","LB","LH","LW","LBU","LHU","ADDI","SLTI","SLTIU","XORI","ORI","ANDI","SLLI","SRLI","SRAI"}},
    {"S_TYPE", {"SB","SH","SW"}},
    {"R_TYPE", {"ADD","SUB","SLL","SLT","SLTU","XOR","SRL","SRA","OR","AND"}}
};

// Reverse mapping instruction -> type
static unordered_map<string,string> INSTRUCTION_TO_TYPE;

// Register names x0..x31
static vector<string> regs;

// RNG
static mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());

// Keep track of used registers for initialization
static set<int> used_registers;

int rnd_int(int a, int b){ uniform_int_distribution<int> d(a,b); return d(rng); }

string to_bin(unsigned value, int bits){
    string s; s.reserve(bits);
    for(int i=bits-1;i>=0;--i) s.push_back(((value>>i)&1)?'1':'0');
    return s;
}

string bin_concat(const vector<string>& parts){ string out; for(auto &p: parts) out += p; return out; }

string bin_to_hex32(const string &bin){
    unsigned long v = 0;
    for(char c: bin){ v = (v<<1) + (c=='1'); }
    ostringstream ss; ss << hex << setfill('0') << setw(8) << (v & 0xFFFFFFFFul);
    return ss.str();
}

void add_instruction(vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list,
                     const string& bin, const string& as){
    bin_list.push_back(bin);
    asm_list.push_back(as);
    hex_list.push_back(bin_to_hex32(bin));
}

// R_TYPE generation
void generate_r(const string &name, const vector<int>& registers_to_use,
                vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list){
    string opcode = OPCODES.at(name);
    string funct3 = FUNCT_CODES.at(name);
    string funct7 = (name=="SUB" || name=="SRA") ? "0100000" : "0000000";

    int rs1 = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];
    int rs2 = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];
    int rd  = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];

    used_registers.insert(rd);  // Track destination register for initialization

    string rs1b = to_bin(rs1,5), rs2b = to_bin(rs2,5), rdb = to_bin(rd,5);
    string bin = bin_concat({funct7, rs2b, rs1b, funct3, rdb, opcode});
    ostringstream as; as << setw(10) << left << name << "   x" << rd << ", x" << rs1 << ", x" << rs2;
    add_instruction(bin_list, asm_list, hex_list, bin, as.str());
}

// I_TYPE generation
void generate_i(const string &name, const vector<int>& registers_to_use, vector<int>& stored_memory_locations,
                vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list){
    string opcode = OPCODES.at(name);
    string funct3 = FUNCT_CODES.at(name);
    int rs1 = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];
    int rd  = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];

    used_registers.insert(rd);  // Track destination register

    string rs1b = to_bin(rs1,5), rdb = to_bin(rd,5);

    if(SHIFT_IMMEDIATE_INSTRUCTION_NAMES.count(name)){
        string imm7 = (name=="SRAI") ? "0100000" : "0000000";
        int shamt = rnd_int(0,31);
        string shamtb = to_bin(shamt,5);
        string bin = bin_concat({imm7, shamtb, rs1b, funct3, rdb, opcode});
        ostringstream as; as << setw(10) << left << name << "   x" << rd << ", x" << rs1 << ", " << shamt;
        add_instruction(bin_list, asm_list, hex_list, bin, as.str());
    } else if(LOAD_INSTRUCTION_NAMES.count(name)){
        rs1 = 0; rs1b = to_bin(rs1,5);
        if(stored_memory_locations.empty()) return;
        int imm = stored_memory_locations[rnd_int(0,(int)stored_memory_locations.size()-1)];
        unsigned imm12 = (unsigned)imm & 0xFFFu;
        string imm12b = to_bin(imm12,12);
        string bin = bin_concat({imm12b, rs1b, funct3, rdb, opcode});
        ostringstream as; as << setw(10) << left << name << "   x" << rd << ", " << imm << "(x" << rs1 << ")";
        add_instruction(bin_list, asm_list, hex_list, bin, as.str());
    } else {
        int imm = rnd_int(0, 4095);
        unsigned imm12 = (unsigned)imm & 0xFFFu;
        string imm12b = to_bin(imm12,12);
        string bin = bin_concat({imm12b, rs1b, funct3, rdb, opcode});
        ostringstream as; as << setw(10) << left << name << "   x" << rd << ", x" << rs1 << ", " << imm;
        add_instruction(bin_list, asm_list, hex_list, bin, as.str());
    }
}

// S_TYPE generation
void generate_s(const string &name, const vector<int>& registers_to_use, vector<int>& stored_memory_locations,
                vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list){
    string opcode = OPCODES.at(name);
    string funct3 = FUNCT_CODES.at(name);
    int rs1 = 0;
    int rs2 = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];
    int imm = 2 * rnd_int(0, 2046);
    unsigned imm12 = (unsigned)imm & 0xFFFu;
    stored_memory_locations.push_back(imm);
    string imm11_5 = to_bin((imm12>>5)&0x7F,7);
    string imm4_0  = to_bin(imm12 & 0x1F,5);
    string rs2b = to_bin(rs2,5), rs1b = to_bin(rs1,5);
    string bin = bin_concat({imm11_5, rs2b, rs1b, funct3, imm4_0, opcode});
    ostringstream as; as << setw(10) << left << name << "   x" << rs2 << ", " << imm << "(x" << rs1 << ")";
    add_instruction(bin_list, asm_list, hex_list, bin, as.str());
}

// SB_TYPE generation
void generate_sb(const string &name, const vector<int>& registers_to_use, int instr_idx, int total_instructions,
                 vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list){
    string opcode = OPCODES.at(name);
    string funct3 = FUNCT_CODES.at(name);
    int rs1 = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];
    int rs2 = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];

    used_registers.insert(rs1);
    used_registers.insert(rs2);

    int imm = instr_idx * 4;
    while(imm == instr_idx * 4){ imm = 2 * rnd_int(0, total_instructions*2); }
    string imm12bit = to_bin((imm>>1)&0xFFF,12);
    string bit11 = imm12bit.substr(0,1);
    string bits10_5 = imm12bit.substr(2,6);
    string bits4_1 = imm12bit.substr(8,4);
    string bit0 = imm12bit.substr(1,1);
    string rs2b = to_bin(rs2,5), rs1b = to_bin(rs1,5);
    string bin = bin_concat({bit11, bits10_5, rs2b, rs1b, funct3, bits4_1, bit0, opcode});
    ostringstream as; as << setw(10) << left << name << "   x" << rs1 << ", x" << rs2 << ", " << imm;
    add_instruction(bin_list, asm_list, hex_list, bin, as.str());
}

// U_TYPE generation
void generate_u(const string &name, const vector<int>& registers_to_use,
                vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list){
    string opcode = OPCODES.at(name);
    int rd = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];

    used_registers.insert(rd);

    unsigned imm20 = (unsigned)rnd_int(0, 1048575);
    string imm20b = to_bin(imm20,20);
    string rdb = to_bin(rd,5);
    string bin = bin_concat({imm20b, rdb, opcode});
    ostringstream as; as << setw(10) << left << name << "   x" << rd << ", " << (imm20<<12);
    add_instruction(bin_list, asm_list, hex_list, bin, as.str());
}

// UJ_TYPE generation
void generate_uj(const string &name, const vector<int>& registers_to_use,
                 int instr_idx, int total_instructions, vector<string>& bin_list,
                 vector<string>& asm_list, vector<string>& hex_list){
    string opcode = OPCODES.at(name);
    int rd = registers_to_use[rnd_int(0, (int)registers_to_use.size()-1)];

    used_registers.insert(rd);

    int imm = instr_idx * 4;
    while(imm == instr_idx * 4){ imm = 2 * rnd_int(0, total_instructions*2); }
    string imm20b = to_bin((imm>>1)&0xFFFFF,20);
    string bit20 = imm20b.substr(0,1);
    string bits10_1 = imm20b.substr(10,10);
    string bit11 = imm20b.substr(9,1);
    string bits19_12 = imm20b.substr(1,8);
    string rdb = to_bin(rd,5);
    string bin = bin_concat({bit20, bits19_12, bit11, bits10_1, rdb, opcode});
    ostringstream as; as << setw(10) << left << name << "   x" << rd << ", " << imm;
    add_instruction(bin_list, asm_list, hex_list, bin, as.str());
}

void generate_instruction(const string &name, const vector<int>& registers_to_use,
                          vector<int>& stored_memory_locations,
                          int instr_idx, int total_instructions,
                          vector<string>& bin_list, vector<string>& asm_list, vector<string>& hex_list){
    string typ = INSTRUCTION_TO_TYPE.at(name);
    if(typ=="R_TYPE") generate_r(name, registers_to_use, bin_list, asm_list, hex_list);
    else if(typ=="I_TYPE") generate_i(name, registers_to_use, stored_memory_locations, bin_list, asm_list, hex_list);
    else if(typ=="S_TYPE") generate_s(name, registers_to_use, stored_memory_locations, bin_list, asm_list, hex_list);
    else if(typ=="SB_TYPE") generate_sb(name, registers_to_use, instr_idx, total_instructions, bin_list, asm_list, hex_list);
    else if(typ=="U_TYPE") generate_u(name, registers_to_use, bin_list, asm_list, hex_list);
    else if(typ=="UJ_TYPE") generate_uj(name, registers_to_use, instr_idx, total_instructions, bin_list, asm_list, hex_list);
}

int main(){
    // Prepare reverse mapping and regs
    for(const auto &kv: TYPES_TO_INSTRUCTION){
        for(const string &iname: kv.second) INSTRUCTION_TO_TYPE[iname] = kv.first;
    }
    for(int i=0; i<32; i++) regs.push_back("x" + to_string(i));

    int TEST_CASES_NUMBER = 0;
    while(TEST_CASES_NUMBER < 1){
        cout << "Enter Number of test cases to produce: ";
        if(!(cin >> TEST_CASES_NUMBER)) return 0;
    }

    for(int test_case=0; test_case<TEST_CASES_NUMBER; ++test_case){
        used_registers.clear();

        int REGISTERS_NUMBER = 0;
        int Instructions_Number = 0;
        while(Instructions_Number < 1){
            cout << "Enter Number of Instructions to produce: ";
            if(!(cin >> Instructions_Number)) return 0;
        }
        while(REGISTERS_NUMBER < 1 || REGISTERS_NUMBER > 32){
            cout << "Enter Number of Registers to use(1 to 32): ";
            if(!(cin >> REGISTERS_NUMBER)) return 0;
        }

        vector<int> REGISTERS_TO_USE; REGISTERS_TO_USE.reserve(REGISTERS_NUMBER);
        for(int i=0; i<REGISTERS_NUMBER; i++) REGISTERS_TO_USE.push_back(rnd_int(1,31));

        vector<int> STORED_MEMORY_LOCATIONS;
        vector<string> Instructions_list_binary, instructions_list_assembly, instructions_list_hex;

        // Generate random instructions to track used registers
        for(int instr=0; instr<Instructions_Number; ++instr){
            string instruction_name;
            vector<string> keys; keys.reserve(INSTRUCTION_TO_TYPE.size());
            for(auto &kv: INSTRUCTION_TO_TYPE) keys.push_back(kv.first);
            instruction_name = keys[rnd_int(0, (int)keys.size()-1)];

            while(LOAD_INSTRUCTION_NAMES.count(instruction_name) && STORED_MEMORY_LOCATIONS.empty()){
                instruction_name = keys[rnd_int(0, (int)keys.size()-1)];
            }
            generate_instruction(instruction_name, REGISTERS_TO_USE, STORED_MEMORY_LOCATIONS, instr, Instructions_Number,
                                 Instructions_list_binary, instructions_list_assembly, instructions_list_hex);
        }

        // Generate ADDI initializations for used registers (except x0 which is always 0)
        vector<string> init_bin_list, init_asm_list, init_hex_list;
        for(int regnum : used_registers){
            if(regnum == 0) continue; // x0 hardwired zero
            int rd = regnum;
            int imm_value = regnum;
            string opcode = OPCODES.at("ADDI");
            string funct3 = FUNCT_CODES.at("ADDI");
            string rs1b = to_bin(0,5);
            string rdb = to_bin(rd,5);
            unsigned imm12 = (unsigned)imm_value & 0xFFFu;
            string imm12b = to_bin(imm12,12);
            string bin = bin_concat({imm12b, rs1b, funct3, rdb, opcode});
            ostringstream as; as << setw(10) << left << "ADDI" << "   x" << rd << ", x0, " << imm_value;
            add_instruction(init_bin_list, init_asm_list, init_hex_list, bin, as.str());
        }

        // Prepend initialization instructions
        Instructions_list_binary.insert(Instructions_list_binary.begin(), init_bin_list.begin(), init_bin_list.end());
        instructions_list_assembly.insert(instructions_list_assembly.begin(), init_asm_list.begin(), init_asm_list.end());
        instructions_list_hex.insert(instructions_list_hex.begin(), init_hex_list.begin(), init_hex_list.end());

        // Write files
        string bin_filename = "binary" + to_string(test_case+1) + ".txt";
        string asm_filename = "assembly" + to_string(test_case+1) + ".s";
        string hex_filename = "hex" + to_string(test_case+1) + ".v";
        ofstream bin_file(bin_filename);
        ofstream asm_file(asm_filename);
        ofstream hex_file(hex_filename);

        asm_file << "test" << (test_case+1) << ".elf:     file format elf32-littleriscv\n\n";
        asm_file << "Disassembly of section .text:\n\n00000000 <main>:\n";
        hex_file << "@00000000\n";

        for(size_t i=0; i<Instructions_list_binary.size(); i++){
            bin_file << Instructions_list_binary[i] << "\n";
            asm_file << std::hex << (i*4) << std::dec << ":     " << instructions_list_hex[i] << "  " << instructions_list_assembly[i] << "\n";

            string hx = instructions_list_hex[i];
            if(hx.size() < 8) hx = string(8 - hx.size(), '0') + hx;
            vector<string> bytes;
            for(int b=0; b<8; b+=2) bytes.push_back(hx.substr(b, 2));
            if(i % 4 == 0 && i != 0) hex_file << "\n";
            hex_file << bytes[3] << " " << bytes[2] << " " << bytes[1] << " " << bytes[0] << " ";
        }

        bin_file.close();
        asm_file.close();
        hex_file.close();

        cout << "FINISHED TEST CASE " << (test_case+1) << "\n";
    }

    return 0;
}
