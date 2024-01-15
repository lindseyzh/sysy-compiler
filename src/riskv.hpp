#pragma once
#include "riskv_util.hpp"

// Visit a raw program
void Visit(const koopa_raw_program_t &program) {
    // Note: "values" and "funcs" are both "koopa_raw_slice_t" type
    assert(program.values.kind == KOOPA_RSIK_VALUE);
    Visit(program.values);
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
    if(func->bbs.len == 0) // do nothing
        return;
    std::cout << "\t.text" << "\n";
    std::cout << "\t.globl " << func->name + 1 << "\n";
    std::cout << func->name + 1 << ":\n";
    assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);

    // calculate FrameSize (the stack frame) and StackTop
    calFrameSize(func);

    if (FrameSize > 0 && FrameSize <= 2048){
        std::cout << "\taddi    sp, sp, -" << FrameSize << "\n";
    }
    else if (FrameSize > 2048){
        std::cout << "\tli      s11, -" << FrameSize << "\n";
        std::cout << "\tadd     sp, sp, s11\n";
    }
    if (saveRA){
        int32_t actualSize = FrameSize - 4;
        if (actualSize >= -2048 && actualSize < 2048){
            std::cout << "\tsw      ra, " << actualSize << "(sp)\n";
        }
        else{
            std::cout << "\tli      s11, " << actualSize << "\n";
            std::cout << "\tadd     s11, sp, s11\n";
            std::cout << "\tsw      ra, (s11)\n";
        }
    }
    // printStackSize(); // for debug

    // Store parameters in the value map
    int32_t stackRagNum = func->params.len > 8 ? 8 : func->params.len;
    for (int32_t i = 0; i < stackRagNum; i++){
        auto ptr = func->params.buffer[i];
        koopa_raw_value_t param = reinterpret_cast<koopa_raw_value_t>(ptr);
        RegInfoMap[param] = RegInfo(i + 7, -1);
        // RegInfoMap[param].printRegInfo(); // for debug
    }
    if(func->params.len > 8){ // If there are >8 parameters, put them on the stack
        for(int32_t i = 8; i < func->params.len; i++){
            auto ptr = func->params.buffer[i];
            koopa_raw_value_t param = reinterpret_cast<koopa_raw_value_t>(ptr);
            RegInfoMap[param] = RegInfo(-1, FrameSize + (i - 8) * 4);
            // RegInfoMap[param].printRegInfo(); // for debug
        }
    }
    
    Visit(func->bbs);
    std::cout << "\n";

    // Reset values
    saveRA = 0;
    StackTop = FrameSize = 0;
    memset(RegStatus, 0, sizeof(RegStatus));
    RegInfoMap.clear();
}

// Visit a basic block
void Visit(const koopa_raw_basic_block_t &bb) {
    assert(bb->name);
    std::cout << bb->name + 1 << ":\n";
    Visit(bb->insts);
}

// Visit an instruction
// Note: type "koopa_raw_value_t" is a pointer of type "koopa_raw_value_data"
RegInfo Visit(const koopa_raw_value_t &value) {
    PreValue = CurValue;
    CurValue = value;

    if (RegInfoMap.count(value)){
        // std::cout << "// RegInfoMap[value].regnum = " << RegInfoMap[value].regnum << "\n";
        if(RegInfoMap[value].regnum == -1){
            int32_t regNum = chooseReg(mid, CurValue);
            int32_t varOffset = RegInfoMap[value].offset;
            RegInfoMap[value].regnum = regNum;
            if (varOffset >= -2048 && varOffset < 2048)
                std::cout << "\tlw      " << RegName[regNum] << ", " << varOffset << "(sp)\n";
            else{
                std::cout << "\tli      s11, " << varOffset << "\n";
                std::cout << "\tadd     s11, sp, s11\n";
                std::cout << "\tlw      " << RegName[regNum] << ", (s11)\n";
            }
        }
        CurValue = PreValue;
        return RegInfoMap[value]; //VarRegMap[value]
    }
    // else

    RegInfo lastReg = RegInfo(-1, -1);

    // Deal with different instructions. See line 383 of koopa.h for the full tag list
    switch (value->kind.tag) {
        case KOOPA_RVT_INTEGER:
            lastReg = Visit(value->kind.data.integer);
            break;
        /// Local memory allocation.
        case KOOPA_RVT_ALLOC:
            // std::cout << "//alloc\n";
            lastReg.offset = StackTop;            
            StackTop += calTypeSize(value->ty->data.pointer.base);
            // printStackSize();
            RegInfoMap[value] = lastReg;
            // printRegStatus();
            break;
        /// Global memory allocation.
        case KOOPA_RVT_GLOBAL_ALLOC:
            GlobalVarTab[value] = Visit(value->kind.data.global_alloc);
            // Note: the following code results in error. Should not define a new variant in "switch()".
            // std::string globalVar = Visit(value->kind.data.global_alloc);
            // GlobalVarTab[value] = globalVar; 
            break;
        /// Memory load.
        case KOOPA_RVT_LOAD:
            // std::cout << "//load\n";
            RegInfoMap[value] = lastReg = Visit(value->kind.data.load);
            // printStackSize();
            // printRegStatus();
            break;
        /// Memory store.   
        case KOOPA_RVT_STORE:
            // std::cout << "//store\n";
            Visit(value->kind.data.store);
            // printRegStatus();
            break;
        /// Pointer calculation.
        case KOOPA_RVT_GET_PTR:
            RegInfoMap[value] = lastReg = Visit(value->kind.data.get_ptr);
            break;
        /// Element pointer calculation.
        case KOOPA_RVT_GET_ELEM_PTR:
            RegInfoMap[value] = lastReg = Visit(value->kind.data.get_elem_ptr);
            break;
        case KOOPA_RVT_BINARY:
            // std::cout << "//binary\n";
            RegInfoMap[value] = lastReg = Visit(value->kind.data.binary);
            // printRegStatus();
            break;
        case KOOPA_RVT_BRANCH:
            Visit(value->kind.data.branch);
            break;  
        case KOOPA_RVT_JUMP:
            // std::cout << "//jump\n";
            Visit(value->kind.data.jump);
            break;
        case KOOPA_RVT_CALL:
            // std::cout << "//call\n";
            RegInfoMap[value] = lastReg = Visit(value->kind.data.call);
            if (value->ty->tag != KOOPA_RTT_UNIT) {
                RegValue[lastReg.regnum] = value;
                RegStatus[lastReg.regnum] = mid;
            }   
            break;
        case KOOPA_RVT_RETURN:
            // std::cout << "//ret\n";
            Visit(value->kind.data.ret);
            // printStackSize();
            // printRegStatus();
            break;
        default:
            assert(false);
    }
    return lastReg;
}

RegInfo Visit(const koopa_raw_return_t &ret){
    int32_t regNum = 0;
    if(ret.value){
        regNum = Visit(ret.value).regnum;
        if(RegName[regNum] != "a0"){
            // std::cout << "// Visit(ret): if(RegName[regNum] != a0)\n"; // for debug
            std::cout << "\tmv      a0, " << RegName[regNum] << "\n";
        }
    }
    resetRegs(0);
    if (saveRA){
        int32_t actualSize = FrameSize - 4;
        if (actualSize >= -2048 && actualSize < 2048)
            std::cout << "\tlw      ra, " << actualSize << "(sp)\n";
        else{
            std::cout << "\tli      t0, " << actualSize << "\n";
            std::cout << "\tadd     t0, sp, t0\n";
            std::cout << "\tlw      ra, (t0)\n";
        }
    }

    if (FrameSize > 0 && FrameSize < 2048)
        std::cout << "\taddi    sp, sp, " << FrameSize << "\n";
    else if (FrameSize > 2047){
        // std::cout << "\tli      t0, -" << FrameSize << "\n"; // Spend one day to find this bug
        std::cout << "\tli      t0, " << FrameSize << "\n";
        std::cout << "\tadd     sp, sp, t0" << "\n";
    }
    std::cout << "\tret\n";
    return RegInfo();
}

// Return the number representing the register
RegInfo Visit(const koopa_raw_integer_t &integer){
    if(integer.value == 0){
        return RegInfo(X0, -1);
    }
    int32_t regNum = chooseReg(low, CurValue);
    moveToReg(integer.value, RegName[regNum]);
    return RegInfo(regNum, -1);
}

int32_t lastStat = -1, lasregnum;

std::string getStringNameForBinary(const koopa_raw_value_t &value){
    RegInfo regInfo = Visit(value);
    if(lastStat != -1)
        RegStatus[lasregnum] = lastStat;
    lastStat = RegStatus[regInfo.regnum];
    RegStatus[regInfo.regnum] = high;
    lasregnum = regInfo.regnum;
    return RegName[lasregnum];
}

RegInfo Visit(const koopa_raw_binary_t &binary){
    lastStat = -1;
    std::string lhsRegName = getStringNameForBinary(binary.lhs);
    std::string rhsRegName = getStringNameForBinary(binary.rhs);
    int32_t ansRegnum = chooseReg(mid, CurValue);
    RegStatus[lasregnum] = lastStat;
    RegInfo ansReg = RegInfo(ansRegnum, -1);

    // std::string lhsRegName = RegName[lhsReg.regnum];
    // std::string rhsRegName = RegName[rhsReg.regnum];
    std::string ansRegName = RegName[ansReg.regnum];

    std::string inst;
    switch(binary.op){
        case KOOPA_RBO_NOT_EQ:
            if (lhsRegName == "x0"){
                std::cout << "\tsnez    " << ansRegName << ", " << rhsRegName << "\n";
            }
            else if (rhsRegName == "x0"){
                std::cout << "\tsnez    " << ansRegName << ", " << lhsRegName << "\n";
            }
            else {
                std::cout << "\txor     " << ansRegName << ", " << lhsRegName << ", "
                          << rhsRegName << "\n";
                std::cout << "\tsnez    " << ansRegName << ", " << ansRegName << "\n";
            }
            return ansReg;
        case KOOPA_RBO_EQ:
            if (lhsRegName == "x0"){
                std::cout << "\tseqz    " << ansRegName << ", " << rhsRegName << "\n";
            }
            else if (rhsRegName == "x0"){
                std::cout << "\tseqz    " << ansRegName << ", " << lhsRegName << "\n";
            }
            else {
                std::cout << "\txor     " << ansRegName << ", " << lhsRegName << ", "
                          << rhsRegName << "\n";
                std::cout << "\tseqz    " << ansRegName << ", " << ansRegName << "\n";
            }
            return ansReg;
        case KOOPA_RBO_GE:
            std::cout << "\tslt     " << ansRegName << ", " << lhsRegName << ", "
                      << rhsRegName << "\n";
            std::cout << "\txori    " << ansRegName << ", " << ansRegName << ", 1\n";
            return ansReg;
        case KOOPA_RBO_LE:
            std::cout << "\tsgt     " << ansRegName << ", " << lhsRegName << ", "
                      << rhsRegName << "\n";
            std::cout << "\txori    " << ansRegName << ", " << ansRegName << ", 1\n";
            return ansReg;
        case KOOPA_RBO_GT:
            inst = "sgt     ";
            break;
        case KOOPA_RBO_LT:
            inst = "slt     ";
            break;
        case KOOPA_RBO_ADD:
            inst = "add     ";
            break;
        case KOOPA_RBO_SUB:
            inst = "sub     ";
            break;
        case KOOPA_RBO_MUL:
            inst = "mul     ";
            break;
        case KOOPA_RBO_DIV:
            inst = "div     ";
            break;
        case KOOPA_RBO_MOD:
            inst = "rem     ";
            break;
        case KOOPA_RBO_AND:
            inst = "and     ";
            break;
        case KOOPA_RBO_OR:
            inst = "or      ";
            break;
        case KOOPA_RBO_XOR:
            inst = "xor     ";
            break;
        case KOOPA_RBO_SHL:
            inst = "sll     ";
            break;
        case KOOPA_RBO_SHR:
            inst = "srl     ";
            break;
        case KOOPA_RBO_SAR:
            inst = "sra     ";
            break;
        default:
            assert(0); 
    }
    std::cout << "\t" << inst << ansRegName << ", " << lhsRegName << ", " << rhsRegName << "\n";

    return ansReg;
}

RegInfo Visit(const koopa_raw_load_t &load){
    int32_t regNum;
    RegInfo regInfo(-1, -1);
    if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        regNum = regInfo.regnum = chooseReg(mid, CurValue);
        std::cout << "\tla      " << RegName[regNum] << ", " << GlobalVarTab[load.src] << "\n";
        std::cout << "\tlw      "<< RegName[regNum] << ", 0(" << RegName[regNum] << ")\n";
    }
    else if (load.src->kind.tag == KOOPA_RVT_GET_PTR || load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
        regNum = regInfo.regnum = chooseReg(high, CurValue);
        RegInfo srcReg = Visit(load.src);
        int32_t srcRegNum = srcReg.regnum;
        std::cout << "\tlw      " << RegName[regNum] << ", (" << RegName[srcRegNum] << ")\n";
        RegStatus[regNum] = mid;
    }
    else if (RegInfoMap[load.src].regnum != -1){ // if already stored in a register, use it
        return RegInfoMap[load.src];
    }
    else{
        regNum = regInfo.regnum = chooseReg(mid, CurValue);
        int32_t varOffset = RegInfoMap[load.src].offset;
        regInfo.offset = varOffset;
        if (varOffset >= -2048 && varOffset < 2048){
            std::cout << "\tlw      " << RegName[regNum] << ", " << varOffset << "(sp)\n";
        }
        else{
            std::cout << "\tli      s11, " << varOffset << "\n";
            std::cout << "\tadd     s11, s11, sp\n";
            std::cout << "\tlw      " << RegName[regNum] << ", (s11)\n";
        }
    }
    return regInfo;
}

RegInfo Visit(const koopa_raw_store_t &store){
    RegInfo regInfo = Visit(store.value);
    int32_t regNum = regInfo.regnum;
    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        std::cout << "\tla      s11, " << GlobalVarTab[store.dest] << "\n";
        std::cout << "\tsw      " << RegName[regNum] << ", 0(s11)\n";
        return regInfo;
    }
    if (store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || store.dest->kind.tag == KOOPA_RVT_GET_PTR){
        int32_t prevStatus = RegStatus[regNum];
        RegStatus[regNum] = high;
        RegInfo destRegInfo = Visit(store.dest);
        RegStatus[regNum] = prevStatus;
        std::cout << "\tsw      " << RegName[regNum] << ", (" << RegName[destRegInfo.regnum] << ")\n";
        return regInfo;
    }

    // printRegStatus(); // for debug
    if (RegInfoMap[store.dest].offset == -1){ // Store it in the stack
        RegInfoMap[store.dest].offset = StackTop;
        StackTop += 4;
    }
    else  {
        for (int32_t i = 0; i < REGNUM; i++){
            if (i != regNum && RegStatus[i] > low && 
                    RegInfoMap[RegValue[i]].offset == RegInfoMap[store.dest].offset){
                RegStatus[i] = low; // These regs are safe to use. No need to store old data. 
                RegInfoMap[RegValue[i]].regnum = regNum;
                // std::cout << "// Reg " << RegName[i] << "\n";
            }
        }
    }
    int32_t varOffset = RegInfoMap[store.dest].offset;
    if (varOffset >= -2048 && varOffset < 2048)
        std::cout << "\tsw      " << RegName[regNum] << ", " << varOffset << "(sp)\n";
    else{
        std::cout << "\tli      s11, " << varOffset << "\n";
        std::cout << "\tadd     s11, s11, sp\n";
        std::cout << "\tsw      " << RegName[regNum] << ", (s11)\n";
    }
    return regInfo;
}

RegInfo Visit(const koopa_raw_branch_t &branch){
    RegInfo regInfo = Visit(branch.cond);
    resetRegs(0);
    std::cout << "\tbnez    " << RegName[regInfo.regnum] << ", " << (branch.true_bb->name + 1) << "\n";
    std::cout << "\tj       " << (branch.false_bb->name + 1) << "\n";
    return regInfo;
}

RegInfo Visit(const koopa_raw_jump_t &jump){
    resetRegs(0);
    std::cout << "\tj     " << (jump.target->name + 1) << "\n";
    return RegInfo();
}

RegInfo Visit(const koopa_raw_call_t &call){
    RegInfo regInfo(A0, -1);
    std::vector<int32_t> oldStatus;

    resetRegs(1);
    int32_t stackRegNum = call.args.len > 8 ? 8 : call.args.len;
    for (int32_t i = 0; i < stackRegNum; i++){
        // RegName[i+7]: the ith parameter
        auto ptr = call.args.buffer[i];
        koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
        RegInfo lastReg = Visit(value);
        int32_t lastRegNum = lastReg.regnum;
        oldStatus.push_back(RegStatus[i + 7]);
        RegStatus[i + 7] = high;
        if (lastRegNum != i + 7)
            std::cout << "\tmv      " << RegName[i + 7] << ", " << RegName[lastRegNum] << "\n";
    }

    if(call.args.len > 8)
        for (int32_t i = 8; i < call.args.len; i++){
            auto ptr = call.args.buffer[i];
            koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
            RegInfo lastReg = Visit(value);
            int32_t lastRegNum = lastReg.regnum;
            int32_t paramOffset = (i - 8) * 4;
            if (paramOffset >= -2048 && paramOffset < 2048)
                std::cout << "\tsw      " << RegName[lastRegNum] << ", " << paramOffset << "(sp)\n";
            else{
                std::cout << "\tli      s11, " << paramOffset << "\n";
                std::cout << "\tadd     s11, s11, sp" << "\n";
                std::cout << "\tsw      " << RegName[lastRegNum] << ", (s11)\n";
            }
        }
    for (int32_t i = 0; i < oldStatus.size(); i++)
        RegStatus[i + 7] = oldStatus[i]; // Status info recovery
    std::cout << "\tcall    " << (call.callee->name + 1) << "\n";
    resetRegs(0);
    return regInfo;
}

std::string Visit(const koopa_raw_global_alloc_t &global_alloc){
    std::string symName = GlobalAllocString + std::to_string(GlobalVarCount++);
    std::cout << "\t.data\n";
    std::cout << "\t.globl " << symName << "\n";
    std::cout << symName << ":\n";
    if(global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT){
        std::cout << "\t.zero " << calTypeSize(global_alloc.init->ty) << "\n";
    }
    else if(global_alloc.init->kind.tag == KOOPA_RVT_INTEGER){
        std::cout << "\t.word " << global_alloc.init->kind.data.integer.value << "\n";
    }
    else if(global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE){
        // For raw aggregate const. See line 215 of koopa.h
        aggregateRecurrence(global_alloc.init);
        std::cout << "\n"; 
    }
    return symName;
}

RegInfo Visit(const koopa_raw_get_ptr_t &ptr){
    RegInfo srcReg = RegInfoMap[ptr.src];
    int32_t curRegNum, srcRegNum = srcReg.regnum;
    int32_t regNum = chooseReg(high, CurValue);
    RegInfo regInfo = {regNum, -1};
    RegInfo indexReg = Visit(ptr.index);
    int32_t indexRegNum = indexReg.regnum;
    int32_t ptrSize = calTypeSize(ptr.src->ty->data.pointer.base);
    if (indexRegNum != X0 && ptrSize != 0){
        int32_t prevStatus = RegStatus[indexRegNum];
        RegStatus[indexRegNum] = high;
        curRegNum = chooseReg(low, CurValue);
        RegStatus[indexRegNum] = prevStatus;
        std::cout << "\tli      " << RegName[curRegNum] << ", " << ptrSize << "\n";
        std::cout << "\tmul     " << RegName[curRegNum] << ", " << RegName[curRegNum] << ", " << RegName[indexRegNum] << "\n";
    }
    else {
        curRegNum = X0;
    }
    std::cout << "\tadd     " << RegName[regNum] << ", " << RegName[srcRegNum] << ", " << RegName[curRegNum] << "\n";
    RegStatus[regNum] = mid;
    return regInfo;
}

RegInfo Visit(const koopa_raw_get_elem_ptr_t &ptr){
    int32_t ptrSize = 
        calTypeSize(ptr.src->ty->data.pointer.base) / ptr.src->ty->data.pointer.base->data.array.len;
    int32_t regNum = chooseReg(high, CurValue), indexRegNum;
    RegInfo regInfo(regNum, -1), indexReg;
    if (ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC){
        indexReg = Visit(ptr.index);
        std::cout << "\tla      " << RegName[regNum] << ", " << GlobalVarTab[ptr.src] << "\n";
        std::cout << "\tli      s11, " << ptrSize << "\n";
        std::cout << "\tmul     s11, s11, " << RegName[indexReg.regnum] << "\n";
        std::cout << "\tadd     " << RegName[regNum] << ", " << RegName[regNum] << ", s11\n";
    }
    
    else{
        RegInfo srcReg = RegInfoMap[ptr.src];
        int32_t curRegNum, srcRegNum = srcReg.regnum, prevStatus;
        bool fromVariable = ptr.src->name && ptr.src->name[0] == '@';
        if(fromVariable){
            int32_t offset = srcReg.offset;
            if (offset >= -2048 && offset < 2048){
                std::cout << "\taddi    " << RegName[regNum] << ", sp, " << offset << "\n";
            }
            else{
                std::cout << "\tli      s11, " << offset << "\n";
                std::cout << "\tadd     " << RegName[regNum] <<", sp, s11\n";
            }
        }
        else{
            srcReg = Visit(ptr.src);
            srcRegNum = srcReg.regnum;
            prevStatus = RegStatus[srcRegNum];
            RegStatus[srcRegNum] = high;
        }

        indexReg = Visit(ptr.index);
        indexRegNum = indexReg.regnum;
        if (indexRegNum != X0 && ptrSize != 0){
            int32_t indexRegNumPrevStatus = RegStatus[indexRegNum];
            RegStatus[indexRegNum] = high;
            curRegNum = chooseReg(low, CurValue);
            RegStatus[indexRegNum] = indexRegNumPrevStatus;
            std::cout << "\tli      " << RegName[curRegNum] << ", " << ptrSize << "\n";
            std::cout << "\tmul     " << RegName[curRegNum] << ", " << RegName[curRegNum] << ", " << 
                                        RegName[indexRegNum] <<"\n";
        }
        else curRegNum = X0;
        
        if (fromVariable){
            std::cout << "\tadd     " << RegName[regNum] << ", " << RegName[regNum] << ", " << 
                                        RegName[curRegNum] << "\n";
        }
        else{
            std::cout << "\tadd     " << RegName[regNum] << ", " << RegName[srcRegNum] << ", " << 
                                        RegName[curRegNum] << "\n";
            RegStatus[srcRegNum] = prevStatus;
        }
    }
    RegStatus[regNum] = mid;
    return regInfo;
}