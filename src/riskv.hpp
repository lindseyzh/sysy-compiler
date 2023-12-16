#pragma once
#include "riskv_helper.hpp"

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
    if(func->bbs.len == 0) return;
    std::cout << "\t.globl " << func->name + 1 << "\n";
    std::cout << func->name + 1 << ":\n";
    assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);

    // calculate FrameSize and StackTop
    cal_frame_size(func);
    if (FrameSize > 0 && FrameSize <= 2048)
        std::cout << "\taddi    sp, sp, -" << FrameSize << "\n";
    else if (FrameSize > 2048){
        std::cout << "\tli      s11, -" << FrameSize << "\n";
        std::cout << "\tadd     sp, sp, s11" << "\n";
    }
    if (saveRA){
        int32_t actualSize = FrameSize - 4;
        if (actualSize >= -2048 && actualSize <= 2047){
            std::cout << "\tsw      ra, " << actualSize << "(sp)\n";
        }
        else{
            std::cout << "\tli      s11, " << actualSize << "\n";
            std::cout << "\tadd     s11, sp, s11\n";
            std::cout << "\tsw      ra, (s11)\n";
        }
    }
    Visit(func->bbs);
    std::cout << "\n";

    // Reset values
    saveRA = 0;
    StackTop = FrameSize = 0;
    for (int i = 0; i < REGNUM; i++)
        RegStatus[i] = 0;
    VarOffsetMap.clear();
    VarRegMap.clear();
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
    PreValue = CurValue;
    CurValue = value;

    if (VarRegMap.count(value)){
        if(VarRegMap[value] == -1){
            int32_t regNum = choose_reg(1, value);
            int32_t varOffset = VarOffsetMap[value];
            VarRegMap[value] = regNum;
            if (varOffset >= -2048 && varOffset <= 2047)
                std::cout << "\tlw      " << RegName[regNum] << ", " << varOffset << "(sp)\n";
            else{
                std::cout << "\tli      s11, " << varOffset << "\n";
                std::cout << "\tadd     s11, sp, s11\n";
                std::cout << "\tlw      " << RegName[regNum] << ", (s11)\n";
        }
        }
        CurValue = PreValue;
        return VarRegMap[value]; //VarRegMap[value]
    }

    const auto &kind = value->kind;
    int32_t retValue = 0;
    // See line 383 of koopa.h for the full tag list
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            print_stack_size();
            retValue = Visit(kind.data.integer);
            break;
        case KOOPA_RVT_ALLOC:
            /// Memory allocation.
            std::cout << "//alloc\n";
            print_stack_size();
            assert(value->ty->tag == KOOPA_RTT_POINTER);
            VarRegMap[value] = -1;
            VarOffsetMap[value] = StackTop;            
            StackTop += calc_type_size(value->ty->data.pointer.base);
            print_stack_size();
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            /// Memory load.
            break;
        case KOOPA_RVT_LOAD:
            /// Memory store.   
            std::cout << "//load\n";
            retValue = Visit(kind.data.load);
            VarRegMap[value] = retValue;
            VarOffsetMap[value] = -1;
            print_stack_size();
            break;
        case KOOPA_RVT_STORE:
            /// Pointer calculation.
            std::cout << "//store\n";
            Visit(kind.data.store);
            break;
        case KOOPA_RVT_BINARY:
            std::cout << "//binary\n";
            print_stack_size();
            retValue = Visit(kind.data.binary);
            VarRegMap[value] = retValue;
            VarOffsetMap[value] = -1;
            print_stack_size();
            break;
        case KOOPA_RVT_RETURN:
            std::cout << "//ret\n";
            print_stack_size();
            Visit(kind.data.ret);
            break;
        default:
            assert(false);
    }
    return retValue;
}

void Visit(const koopa_raw_return_t &ret){
    int32_t regNum = 0;
    if(ret.value){
        regNum = Visit(ret.value);
    }
    reset_regs(0);
    if (saveRA){
        if (FrameSize - 4 >= -2048 && FrameSize - 4 <= 2047)
            std::cout << "\tlw      ra, " << FrameSize - 4 << "(sp)\n";
        else{
            std::cout << "\tli      t0, " << FrameSize - 4 << "\n";
            std::cout << "\tadd     t0, sp, t0\n";
            std::cout << "\tlw      ra, (t0)\n";
        }
    }

    if(RegName[regNum] != "a0"){
        std::cout << "\tmv      a0, " << RegName[regNum] << "\n";
    }
    if (FrameSize > 0 && FrameSize <= 2048)
        std::cout << "\taddi    sp, sp, " << FrameSize << "\n";
    else if (FrameSize > 2048){
        std::cout << "\tli      t0, -" << FrameSize << "\n";
        std::cout << "\tadd     sp, sp, t0" << "\n";
    }
    std::cout << "\tret\n";
    return;
}

// Return the number representing the register
int32_t Visit(const koopa_raw_integer_t &integer){
    int32_t val = integer.value;
    if(val == 0){
        return X0;
    }
    int32_t regnum = choose_reg(0, CurValue);
    std::string reg = RegName[regnum];
    mv_to_reg(val, reg);
    return regnum;
}

int32_t Visit(const koopa_raw_binary_t &binary){
    int32_t lasstat; // a tmp variant
    int32_t lhs_regnum = Visit(binary.lhs);
    lasstat = RegStatus[lhs_regnum];
    RegStatus[lhs_regnum] = 2;
    int32_t rhs_regnum = Visit(binary.rhs);
    RegStatus[lhs_regnum] = lasstat;
    lasstat = RegStatus[rhs_regnum];
    RegStatus[rhs_regnum] = 2;
    int32_t ans_regnum = choose_reg(1, CurValue);
    std::string lhs_reg = RegName[lhs_regnum];
    std::string rhs_reg = RegName[rhs_regnum];
    std::string ans_reg = RegName[ans_regnum];
    // TODO: register allocation. 
    switch(binary.op){
        case KOOPA_RBO_NOT_EQ:
            std::cout << "\txor     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\tsnez    " << ans_reg << ", " << ans_reg << "\n";
            break;
        case KOOPA_RBO_EQ:
            std::cout << "\txor     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\tseqz    " << ans_reg << ", " << ans_reg << "\n";
            break;
        case KOOPA_RBO_GT:
            std::cout << "\tsgt     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_LT:
            std::cout << "\tslt     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_GE:
            std::cout << "\tslt     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\txori    " << ans_reg << ", " << ans_reg << ", 1\n";
            break;
        case KOOPA_RBO_LE:
            std::cout << "\tsgt     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            std::cout << "\txori    " << ans_reg << ", " << ans_reg << ", 1\n";
            break;
        case KOOPA_RBO_ADD:
            std::cout << "\tadd     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SUB:
            std::cout << "\tsub     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_MUL:
            std::cout << "\tmul     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_DIV:
            std::cout << "\tdiv     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_MOD:
            std::cout << "\trem     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_AND:
            std::cout << "\tand     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_OR:
            std::cout << "\tor      " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_XOR:
            std::cout << "\txor     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SHL:
            std::cout << "\tsll     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SHR:
            std::cout << "\tsrl     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        case KOOPA_RBO_SAR:
            std::cout << "\tsra     " << ans_reg << ", " << lhs_reg << ", "
                << rhs_reg << "\n";
            break;
        default:
            assert(false); 
    }
    return ans_regnum;
}

int32_t Visit(const koopa_raw_load_t &load){
    koopa_raw_value_t src = load.src;
    // if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    //     int32_t regNum = choose_reg(1, CurValue);
    //     std::cout << "\tla " << RegName[regNum] << ", " <<
    //         global_values[src] << "\n";
    //     std::cout << "\tlw "<< RegName[regNum] << ", 0(" <<
    //         RegName[regNum] << ")\n";
    //     return regNum;
    // }
    // else if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src->kind.tag == KOOPA_RVT_GET_PTR) {
    //     struct Reg result_var = {choose_reg(2), -1};
    //     struct Reg src_var = Visit(load.src);
    //     reg_stats[result_var.regNum] = 1;
    //     std::cout << "\tlw " << RegName[result_var.regNum] << ", (" <<
    //         RegName[src_var.regNum] << ")" << std::endl;
    //     return result_var;
    // }

    if (VarRegMap[src] >= 0){ // if stored in a register
        return VarRegMap[src];
    }
    int32_t regNum = choose_reg(1, CurValue);
    int32_t varOffset = VarOffsetMap[src];
    if (varOffset >= -2048 && varOffset <= 2047){
        std::cout << "\tlw      " << RegName[regNum] << ", " << varOffset << "(sp)\n";
    }
    else{
        std::cout << "\tli      s11, " << varOffset << "\n";
        std::cout << "\tadd     s11, s11, sp\n";
        std::cout << "\tlw      " << RegName[regNum] << ", (s11)\n";
    }
    return regNum;
}

void Visit(const koopa_raw_store_t &store)
{
    int32_t regNum = Visit(store.value);
    koopa_raw_value_t dest = store.dest;
    // if (dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
    //     std::cout << "\tla    s11, " << global_values[dest] << std::endl;
    //     std::cout << "\tsw    " << reg_names[value.reg_name] << ", 0(s11)" <<
    //         std::endl;
    //     return;
    // }
    // else if (dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
    //     dest->kind.tag == KOOPA_RVT_GET_PTR)
    // {
    //     int old_stat = reg_stats[value.reg_name];
    //     reg_stats[value.reg_name] = 2;
    //     struct Reg dest_var = Visit(dest);
    //     assert(dest_var.reg_name >= 0);
    //     reg_stats[value.reg_name] = old_stat;
    //     std::cout << "\tsw    " << reg_names[value.reg_name] << ", (" <<
    //         reg_names[dest_var.reg_name] << ")" << std::endl;
    //     return;
    // }
    assert(VarRegMap.count(dest));
    if (VarOffsetMap[dest] == -1){
        VarOffsetMap[dest] = StackTop;
        StackTop += 4;
    }
    else  {
        for (int i = 0; i < REGNUM; i++){
            if (i == regNum)
                continue;
            else if (RegStatus[i] > 0 && VarOffsetMap[RegValue[i]] == VarOffsetMap[dest]){
                RegStatus[i] = 0;  
                VarRegMap[RegValue[i]] = regNum;
            }
        }
    }
    int32_t varOffset = VarOffsetMap[dest];
    if (varOffset >= -2048 && varOffset <= 2047)
        std::cout << "\tsw      " << RegName[regNum] << ", " << varOffset << "(sp)\n";
    else{
        std::cout << "\tli      s11, " << varOffset << "\n";
        std::cout << "\tadd     s11, s11, sp\n";
        std::cout << "\tsw      " << RegName[regNum] << ", (s11)\n";
    }
}
