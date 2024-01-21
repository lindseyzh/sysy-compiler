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

const std::string GlobalAllocString = "_global_alloc_";

class RegInfo {
    public:
        int32_t regnum = -1;
        int32_t offset = -1; 

        RegInfo(){};
        RegInfo(int32_t rn, int32_t ofs): regnum(rn), offset(ofs) {}

        inline void printRegInfo(){
            std::cout << "// regnum = " << regnum << ", offset = " << offset << "\n";
            return;
        }
};
std::unordered_map<koopa_raw_value_t, RegInfo> RegInfoMap;

// std::unordered_map<koopa_raw_value_t, int32_t> VarOffsetMap;
// std::unordered_map<koopa_raw_value_t, int32_t> VarRegMap;
koopa_raw_value_t RegValue[16];
std::string RegName[16] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1", "a2",
                           "a3", "a4", "a5", "a6", "a7", "x0"};
int32_t RegStatus[16] = {};
// std::unordered_map<std::string, int32_t> RegNumber {{"t0", 0}, {"t1", 1},  {"t2", 2} , {"t3", 3}, {"t4, 4"}, {"t5", 5}, {"t6", 6},
//     {"a0", 7}, {"a1", 8}, {"a2", 9}, {"a3", 10}, {"a4", 11}, {"a5", 12}, {"a6", 13}, 
//     {"a7", 14},  {"x0", 15} };

int32_t FrameSize = 0;
int32_t StackTop = 0;
uint32_t maxArgNum;
koopa_raw_value_t CurValue, PreValue;
bool saveRA = 0;
int32_t lastOffset = -1;

int32_t GlobalVarCount = 0;
std::unordered_map<koopa_raw_value_t, std::string> GlobalVarTab;

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
RegInfo Visit(const koopa_raw_value_t &value);
RegInfo Visit(const koopa_raw_return_t &ret);
RegInfo Visit(const koopa_raw_integer_t &integer);
RegInfo Visit(const koopa_raw_binary_t &binary);
RegInfo Visit(const koopa_raw_load_t &load);
RegInfo Visit(const koopa_raw_store_t &store);
RegInfo Visit(const koopa_raw_branch_t &branch);
RegInfo Visit(const koopa_raw_jump_t &jump);
RegInfo Visit(const koopa_raw_call_t &call);
std::string Visit(const koopa_raw_global_alloc_t &global_alloc);
RegInfo Visit(const koopa_raw_get_elem_ptr_t &ptr);
RegInfo Visit(const koopa_raw_get_ptr_t &ptr);

// TODO: remove "inline"

void moveToReg(koopa_raw_value_t val, std::string reg){
    if (val->kind.tag == KOOPA_RVT_INTEGER)
        std::cout << "\tli      " << reg << ", " << val->kind.data.integer.value << "\n";
}

void moveToReg(int32_t val, std::string reg){
    std::cout << "\tli      " << reg << ", " << val << "\n";
}

void moveToReg(std::string reg1, std::string reg2){
    std::cout << "\tmv      " << reg2 << ", " << reg1 << "\n";
}

enum Priority {low = 0, mid = 1, high = 2};

int32_t chooseReg(int32_t stat, koopa_raw_value_t value){
    // Search for a reg with low priority data
    for(int32_t i = 0; i < REGNUM - 1; i++)
        if(RegStatus[i] == low){
            RegStatus[i] = stat;
            RegValue[i] = value;
            return i;
        }

    // Search for a reg with mid priority data
    for(int32_t i = 0; i < REGNUM - 1; i++){
        if(RegStatus[i] == mid){
            koopa_raw_value_t preVal = RegValue[i];
            RegInfoMap[preVal].regnum = -1;
            int32_t varOffset = RegInfoMap[preVal].offset;
            // std::cout << "//varOffset=" << varOffset << :\n";
            if (varOffset == -1){
                varOffset = StackTop;
                RegInfoMap[preVal].offset = varOffset;
                StackTop += 4;
            }
            // Note: A correct offset will not be -1 here, so no need to check it.
            if (varOffset >= -2048 && varOffset < 2048){
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
    return -1;
}

int32_t calTypeSize(const koopa_raw_type_t &ty){
    switch(ty->tag){
        case KOOPA_RTT_UNIT:
            return 0;
        case KOOPA_RTT_ARRAY:
            return calTypeSize(ty->data.array.base) * ty->data.array.len;
        case KOOPA_RTT_INT32:
        case KOOPA_RTT_POINTER:
        default:
            return 4;
    }
    assert(false);
    return 0;
}

int32_t calInsSize(const koopa_raw_value_t &value)
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
        return calTypeSize(value->ty->data.pointer.base);
    return calTypeSize(value->ty);
}

int32_t calBlockSize(const koopa_raw_basic_block_t &bb)
{
    // TODO: did not count params?
    int32_t size = 0;
    for (int32_t i = 0; i < bb->insts.len; i++){
        const void *value = bb->insts.buffer[i];
        if(((koopa_raw_value_t)value)->kind.tag == KOOPA_RVT_CALL)
            saveRA = 1;
        size += calInsSize((koopa_raw_value_t)value);
    }
    return size;
}

void calFrameSize(const koopa_raw_function_t &func){
    FrameSize = StackTop = maxArgNum = 0;
    for (int32_t i = 0; i < func->bbs.len; i++){
        auto ptr1 = func->bbs.buffer[i];
        // FrameSize += calBlockSize((koopa_raw_basic_block_t)ptr1);
        koopa_raw_basic_block_t bb = reinterpret_cast<koopa_raw_basic_block_t>(ptr1);
        for (int32_t j = 0; j < bb->insts.len; j++){
            auto ptr2 = bb->insts.buffer[j];
            koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(ptr2);
            if (inst->ty->tag != KOOPA_RTT_UNIT){
                if (inst->kind.tag == KOOPA_RVT_ALLOC)
                    FrameSize += calTypeSize(inst->ty->data.pointer.base);
                else FrameSize += 4;
            }
            if (inst->kind.tag == KOOPA_RVT_CALL){
                if (inst->kind.data.call.args.len > maxArgNum)
                    maxArgNum = inst->kind.data.call.args.len;
                saveRA = 1;
            }
        }
    }
    
    if(maxArgNum > 8){ // Argument number > 8. Put some in the stack.
        FrameSize += (maxArgNum - 8) * 4;
        StackTop += (maxArgNum - 8) * 4;
    }
    FrameSize += saveRA ? 4 : 0;
    FrameSize = (FrameSize + 15) / 16 * 16;
    return;
}

void resetRegs(bool store_to_stack){
    for (int32_t i = 0; i < REGNUM - 1; i++)
        if (RegStatus[i] > 0){
            RegStatus[i] = 0;
            koopa_raw_value_t preVal = RegValue[i];
            RegInfoMap[preVal].regnum = -1;
            int32_t varOffset = RegInfoMap[preVal].offset;
            if (varOffset == -1){
                RegInfoMap[preVal].offset = varOffset = StackTop;
                StackTop += 4;
                if (store_to_stack){
                    if (varOffset >= -2048 && varOffset < 2048)
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

inline void printStackSize(){
    std::cout << "// StackTop=" << StackTop << ", FrameSize=" << FrameSize << "\n";
    return;
}

inline void printRegStatus(){
    return;
    std::cout << "// ";
    for(int i = 0; i < REGNUM; i++)
        std::cout << RegStatus[i] << ",";
    std::cout << "\n";
}

void aggregateRecurrence(const koopa_raw_value_t &aggr){
    koopa_raw_slice_t elems = aggr->kind.data.aggregate.elems;
    for (int32_t i = 0; i < elems.len; i++){
        auto value = reinterpret_cast<koopa_raw_value_t>(elems.buffer[i]);
        if (value->kind.tag == KOOPA_RVT_INTEGER)
            std::cout << "\t.word " << value->kind.data.integer.value << "\n";
        else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
            aggregateRecurrence(value);
    }
}