#pragma once
#include<iostream>
#include<string>
#include<cstring>
#include<cassert>
#include<map>
#include "koopa.h"

#define REGNUM 16
#define X0 15

std::string RegName[16] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0"};
int32_t RegStatus[16] = {};
// std::map<std::string, int32_t> RegNumber {{"t0", 0}, {"t1", 1},  {"t2", 2} , {"t3", 3}, {"t4, 4"}, {"t5", 5}, {"t6", 6},
//     {"a0", 7}, {"a1", 8}, {"a2", 9}, {"a3", 10}, {"a4", 11}, {"a5", 12}, {"a6", 13}, 
//     {"a7", 14},  {"x0", 15} };

inline void mv_to_reg(koopa_raw_value_t val, std::string reg);
inline void mv_to_reg(int32_t val, std::string reg);
inline void mv_to_reg(std::string reg1, std::string reg2);

inline int32_t choose_reg();

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
// Reg Visit(const koopa_raw_value_t &value);
int32_t Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
// Reg Visit(const koopa_raw_integer_t &integer);
int32_t Visit(const koopa_raw_integer_t &integer);
int32_t Visit(const koopa_raw_binary_t &binary);

// Visit a raw program
void Visit(const koopa_raw_program_t &program) {
    // Note: "values" and "funcs" are both "koopa_raw_slice_t" type
    assert(program.values.kind == KOOPA_RSIK_VALUE);
    Visit(program.values);

    std::cout << "\t.text" << "\n";
    assert(program.funcs.kind == KOOPA_RSIK_FUNCTION);
    Visit(program.funcs);
}

// Visit a raw slice
void Visit(const koopa_raw_slice_t &slice) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                assert(false);
        }
    }
}

// Visit a function
void Visit(const koopa_raw_function_t &func) {
    std::cout << "\t.globl " << func->name + 1 << "\n";
    if(func->bbs.len == 0) return;
    std::cout << func->name + 1 << ":\n";
    assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
    for (size_t j = 0; j < func->bbs.len; j++) {
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];
    }
    Visit(func->bbs);
}

// Visit a basic block
void Visit(const koopa_raw_basic_block_t &bb) {
    // if(bb->name)
    //     std::cout << bb->name + 1 << ":" << std::endl;
    Visit(bb->insts);
}

// Visit an instruction
// Note: type "koopa_raw_value_t" is a pointer of type "koopa_raw_value_data"
int32_t Visit(const koopa_raw_value_t &value) {
    const auto &kind = value->kind;
    int retValue = 0;
    // See line 383 of koopa.h for the full tag list
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            retValue = Visit(kind.data.integer);
            break;
        case KOOPA_RVT_BINARY:
            retValue = Visit(kind.data.binary);
            break;
        default:
            assert(false);
    }
    return retValue;
}

void Visit(const koopa_raw_return_t &ret){
    int32_t regnum = 0;
    if(ret.value){
        regnum = Visit(ret.value);
    }
    if(RegName[regnum] != "a0"){
        std::cout << "\tmv a0, " << RegName[regnum] << "\n";
    }
    std::cout << "\tret\n";
    return;
}

// Return 
int32_t Visit(const koopa_raw_integer_t &integer){
    int32_t val = integer.value;
    if(val == 0){
        return X0;
    }
    int32_t regnum = choose_reg();
    std::string reg = RegName[regnum];
    mv_to_reg(val, reg);
    return regnum;
}

// Return the number of register
int32_t Visit(const koopa_raw_binary_t &binary){
    int32_t lhs_regnum = Visit(binary.lhs);
    int32_t rhs_regnum = Visit(binary.rhs);
    int32_t ans_regnum = choose_reg();
    std::string lhs_reg = RegName[lhs_regnum];
    std::string rhs_reg = RegName[rhs_regnum];
    std::string ans_reg = RegName[ans_regnum];
    // TODO: register allocation. 
    switch(binary.op){
        case KOOPA_RBO_NOT_EQ:
            std::cout << "\txor " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\tsnez " << ans_reg << ", " << ans_reg << "\n";
            break;
        case KOOPA_RBO_EQ:
            std::cout << "\txor " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\tseqz " << ans_reg << ", " << ans_reg << "\n";
            break;
        case KOOPA_RBO_GT:
            std::cout << "\tsgt " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_LT:
            std::cout << "\tslt " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_GE:
            std::cout << "\tslt " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\txori " << ans_reg << ", " << ans_reg << ", 1\n";
            break;
        case KOOPA_RBO_LE:
            std::cout << "\tsgt " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\txori " << ans_reg << ", " << ans_reg << ", 1\n";
            break;
        case KOOPA_RBO_ADD:
            std::cout << "\tadd " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SUB:
            std::cout << "\tsub " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_MUL:
            std::cout << "\tmul " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_DIV:
            std::cout << "\tdiv " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_MOD:
            std::cout << "\trem " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_AND:
            std::cout << "\tand " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_OR:
            std::cout << "\tor " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_XOR:
            std::cout << "\txor " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SHL:
            std::cout << "\tsll " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SHR:
            std::cout << "\tsrl " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SAR:
            std::cout << "\tsra " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        default:
            assert(false); 
    }
    return ans_regnum;
}

inline void mv_to_reg(koopa_raw_value_t val, std::string reg){
    if (val->kind.tag == KOOPA_RVT_INTEGER)
        std::cout << "\tli " << reg << ", " << val->kind.data.integer.value << "\n";
}

inline void mv_to_reg(int32_t val, std::string reg){
    std::cout << "\tli " << reg << ", " << val << "\n";
}

inline void mv_to_reg(std::string reg1, std::string reg2){
    std::cout << "\tmv " << reg2 << ", " << reg1 << "\n";
}

int32_t now = 0;

inline int32_t choose_reg(){
    int32_t noww = now;
    now = (now + 1) % (REGNUM - 1);
    return noww;
    // for(int i = 0; i < REGNUM - 1; i++){
    //     if(RegStatus[i] == 0){
    //         RegStatus[i] = 1;
    //         return i;
    //     }
    // }
    // for(int i = 0; i < REGNUM - 1; i++){
    //     if(RegStatus[i] == 1){
    //         RegStatus[i] = 2;
    //         return i;
    //     }
    // }
    return -1;
}

