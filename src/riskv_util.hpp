#pragma once
#include<iostream>
#include<string>
#include<cstring>
#include<cassert>
#include<map>
#include<unordered_map>
#include<cmath>
#include "koopa.h"

#define REGNUM 16
#define X0 15
#define A0 7

// In order to implement register allocation, this map should be modified to map into a union of "int32_t, register"
std::unordered_map<koopa_raw_value_t, int32_t> VarOffsetMap;
std::unordered_map<koopa_raw_value_t, int32_t> VarRegMap;
koopa_raw_value_t RegValue[16];
std::string RegName[16] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0"};
int32_t RegStatus[16] = {};
// std::map<std::string, int32_t> RegNumber {{"t0", 0}, {"t1", 1},  {"t2", 2} , {"t3", 3}, {"t4, 4"}, {"t5", 5}, {"t6", 6},
//     {"a0", 7}, {"a1", 8}, {"a2", 9}, {"a3", 10}, {"a4", 11}, {"a5", 12}, {"a6", 13}, 
//     {"a7", 14},  {"x0", 15} };

int32_t FrameSize = 0;
int32_t StackTop = 0;
uint32_t maxArgNum;
koopa_raw_value_t CurValue, PreValue;
bool saveRA = 0;

int32_t GlobalVarCount = 0;
std::unordered_map<koopa_raw_value_t, std::string> GlobalVarTab;

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
int32_t Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
void Visit(const koopa_raw_call_t &call);
std::string Visit(const koopa_raw_global_alloc_t &global_alloc);

inline void mv_to_reg(koopa_raw_value_t val, std::string reg){
    if (val->kind.tag == KOOPA_RVT_INTEGER)
        std::cout << "\tli      " << reg << ", " << val->kind.data.integer.value << "\n";
}

inline void mv_to_reg(int32_t val, std::string reg){
    std::cout << "\tli      " << reg << ", " << val << "\n";
}

inline void mv_to_reg(std::string reg1, std::string reg2){
    std::cout << "\tmv      " << reg2 << ", " << reg1 << "\n";
}

inline int32_t choose_reg(int32_t stat, koopa_raw_value_t value){
    // Search status 0
    for(int i = 0; i < REGNUM - 1; i++)
        if(RegStatus[i] == 0){
            RegStatus[i] = stat;
            RegValue[i] = value;
            return i;
        }

    // Search status 1
    for(int i = 0; i < REGNUM - 1; i++){
        if(RegStatus[i] == 1){
            koopa_raw_value_t preVal = RegValue[i];
            VarRegMap[preVal] = -1;
            if(!VarOffsetMap.count(preVal)){
                VarOffsetMap[preVal] = -1;
            }
            int32_t varOffset = VarOffsetMap[preVal];
            // std::cout << "//varOffset=" << varOffset << std::endl;
            if (varOffset == -1){
                varOffset = StackTop;
                VarOffsetMap[preVal] = varOffset;
                StackTop += 4;
            }
            if (varOffset >= -2048 && varOffset <= 2047){
                std::cout << "\tsw      " << RegName[i] << ", " << varOffset <<"(sp)\n";
            }
            else{
                std::cout << "\tli      s11, " << varOffset << "\n";
                std::cout << "\tadd     s11, s11, sp\n";
                std::cout << "\tsw      " << RegName[i] << ", (s11)\n";
            }
            RegValue[i] = value;
            RegStatus[i] = stat;
            return i;
        }
    }
    // Fail.
    assert(false);
    return -1;
}

inline int32_t calc_type_size(const koopa_raw_type_t &ty)
{
    switch(ty->tag){
        case KOOPA_RTT_UNIT:
            return 0;
        case KOOPA_RTT_ARRAY:
            return calc_type_size(ty->data.array.base) * ty->data.array.len;
        case KOOPA_RTT_INT32:
        case KOOPA_RTT_POINTER:
        default:
            return 4;
    }
    assert(false);
    return 0;
}

inline int32_t calc_inst_size(const koopa_raw_value_t &value)
{
    if (value->kind.tag == KOOPA_RVT_CALL){
        // maxArgNum = max(maxArgNum, value->kind.data.call.args.len);
        uint32_t argNum = value->kind.data.call.args.len;
        if (argNum > maxArgNum)
            maxArgNum = argNum;
    }
    if(value->ty->tag == KOOPA_RTT_UNIT)
        return 0;
    if(value->kind.tag == KOOPA_RVT_ALLOC)
        return calc_type_size(value->ty->data.pointer.base);
    return calc_type_size(value->ty);
}

inline int32_t calc_bb_size(const koopa_raw_basic_block_t &bb)
{
    // TODO: did not count params?
    int32_t size = 0;
    for (int32_t i = 0; i < bb->insts.len; i++){
        const void *value = bb->insts.buffer[i];
        if(((koopa_raw_value_t)value)->kind.tag == KOOPA_RVT_CALL)
            saveRA = 1;
        size += calc_inst_size((koopa_raw_value_t)value);
    }
    return size;
}

inline void cal_frame_size(const koopa_raw_function_t &func){
    FrameSize = StackTop = 0;
    maxArgNum = 0;
    for (int32_t i = 0; i < func->bbs.len; i++){
        FrameSize += calc_bb_size((koopa_raw_basic_block_t)func->bbs.buffer[i]);
    }
    
    if(maxArgNum > 8){
        int32_t argSize = (maxArgNum - 8) * 4;
        FrameSize += argSize;
        StackTop += argSize;
    }
    // TODO: change FrameSize calculation
    FrameSize += saveRA ? 4 : 0;
    FrameSize = (FrameSize + 15) / 16 * 16;
    return;
}

inline void reset_regs(bool store_to_stack)
{
    for (int32_t i = 0; i < REGNUM - 1; i++)
        if (RegStatus[i] > 0){
            RegStatus[i] = 0;
            auto preVal = RegValue[i];
            VarRegMap[preVal] = -1;
            int32_t varOffset = VarOffsetMap[preVal];
            if (varOffset == -1){
                varOffset = StackTop;
                VarOffsetMap[preVal] = varOffset;
                StackTop += 4;
                if (store_to_stack){
                    if (varOffset >= -2048 && varOffset <= 2047)
                        std::cout << "\tsw      " << RegName[i] << ", " <<
                            varOffset << "(sp)\n";
                    else{
                        std::cout << "\tli      s11, " << varOffset << "\n";
                        std::cout << "\tadd     s11, s11, sp\n";
                        std::cout << "\tsw      " << RegName[i] << ", (s11)\n";
                    }
                }
            }
        }
}

inline void print_stack_size(){
    return;
    std::cout << "// StackTop=" << StackTop << ", FrameSize=" << FrameSize << std::endl;
}

inline void print_reg_status(){
    std::cout << "// ";
    for(int i = 0; i < REGNUM; i++)
        std::cout << RegStatus[i] << ",";
    std::cout << std::endl;
}
