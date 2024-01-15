#pragma once
#include"AST_util.hpp"

// Variants for in AST generation.
// Note: Linking error occurs if I put them in AST_util.hpp, even with #pragma once.

static const std::string thenString = "\%_then_";
static const std::string elseString = "\%_else_";
static const std::string endString = "\%_end_"; 
// Note: there is a case with a variable named "end".
static const std::string whileEntryString = "\%_while_entry_";
static const std::string whileBodyString = "\%_while_body_";
static const std::string whileEndString = "\%_while_end_";

static int32_t varNum = 0;
static int32_t ifElseNum = 0, whileNum = 0;
static bool isRetInt = 1;
static int32_t varArrayNum = 0, constArrayNum = 0;

static std::vector<SymTabType> symTabs;
static std::unordered_map<std::string, bool> isParam; // Is this symbol a parameter name?
static std::unordered_map<std::string, int32_t> symNameCount; // Map an ident to its count
static std::vector<int32_t> whileTab; // Recording current while layers
static std::unordered_map<std::string, FuncInfo> FuncInfoList; // Map a function name to its
static std::unordered_map<std::string, int32_t> ArrayDimension; // The dimension of a given array.
static std::unordered_map<std::string, bool> isArrayGlobal; // To exclude array parameter
static std::vector<int32_t> ArrayInitVals;

static inline SymTabEntry symTabs_lookup(std::string lval);
static std::string getNewSymName();
static void libFuncDecl();

static std::string getArrayType(const std::vector<int32_t> *lens, int32_t d);
static int32_t getArraySize(std::vector<int32_t> offset, int32_t now_num);
static std::string ArrayInit(std::vector<int32_t> lens, std::string lastSymName, int32_t d, int32_t isVarDef);
static void printArrayInit(std::vector<int32_t> lens, int32_t d, int32_t isVarDef);
static std::vector<int32_t> getProduct(std::vector<int32_t> vec);

// Base class for all ASTs
class BaseAST {
    public:
    bool isEmptyInitArray = 0; // For InitValAST. If the Initialization is like "int[10] a = {}".
    int32_t arrayDimension = -1; // For FuncFParam. Set to the array dimension if the parameter is an array

    virtual ~BaseAST() = default;

    virtual std::string Dump() const{ assert(0); return 0; }
    virtual std::string DumpArray() const{ assert(0); return 0; } // called in Dump(), for lv9
    virtual std::string DumpGlobal() const{ assert(0); return ""; }
    virtual std::string DumpGlobalArray() const{ assert(0); return ""; }
    virtual int32_t CalValue() const{ assert(0); return -1; }
    virtual std::string GetIdent() const{ assert(0); return ""; }
    virtual std::string GetType() const{ assert(0); return ""; }
    virtual std::string TypeOfAST() const{ return ""; }
    virtual std::vector<int32_t> GetArrayInitVals(std::vector<int32_t>) const{ 
        assert(0); return std::vector<int32_t>(); }
    virtual bool GetIsArray() const { assert(0); return 0;}
};

class CompUnitAST : public BaseAST {
    public:
        MulVecType funcDefs;
        MulVecType decls;
        std::string Dump() const override {
            libFuncDecl(); // Generate declaration for library functions
            SymTabType globalSymTab;
            symTabs.push_back(globalSymTab);
            for (auto&& it : decls)
                it->DumpGlobal();            
            for (auto&& it : funcDefs)
                it->Dump();            
            symTabs.pop_back();
            return "";
        }
};

class FuncDefAST : public BaseAST {
    public:
        std::string funcType;
        std::string ident;
        BaseASTPtr block;
        MulVecType funcFParams;

        std::string Dump() const override {
            // std::string funcName = "@" + ident;
            FuncInfo funcInfo;
            funcInfo.retType = funcType;
            std::cout << "fun @" << ident << "(";

            // for (int32_t i = 0; i < 0; i++){ // for debug
            for (int32_t i = 0; i < funcFParams.size(); i++){
                ParamInfo paramInfo;
                paramInfo.ident = funcFParams[i]->GetIdent();
                paramInfo.name = funcFParams[i]->Dump();
                paramInfo.type = funcFParams[i]->GetType();
                if (paramInfo.type != "i32"){
                    std::string paramName = paramInfo.name;
                    paramName[0] = '%';
                    ArrayDimension[paramName] = funcFParams[i]->arrayDimension;
                }
                funcInfo.paramInfoList.push_back(paramInfo);
                std::cout << ": " << paramInfo.type;
                if (i != funcFParams.size() - 1)
                    std::cout << ", ";
            }
            std::cout << ")";
            FuncInfoList[ident] = funcInfo;
            if(funcType == "int")
                std::cout << ": i32";
            else assert(funcType == "void");
            std::cout << " {" << "\n";

            std::cout << "%entry_" << ident << ":\n";

            isRetInt = (funcType == "int");
                
            std::string blockEnd = block->Dump();
            if (blockEnd != "ret"){
                if (funcType == "int")
                    std::cout << "\tret 0\n";
                else {
                    assert(funcType == "void");
                    std::cout << "\tret\n";
                }
            }
            std::cout << "}\n\n";
            return blockEnd;
        }

        std::string GetIdent() const override{ 
            return ident; 
        }
};

class FuncFParamAST : public BaseAST {
    public:
        enum {def_common, def_array} def;
        std::string bType;
        std::string ident;
        MulVecType constExpArray;

        std::string Dump() const override{
            if(symNameCount.count(ident) == 0){
                symNameCount[ident] = 0;
            }
            std::string symName = "@" + ident + "_" + std::to_string(symNameCount[ident]++);
            std::cout << symName;
            return symName;
        }
        std::string GetIdent() const override{
            return ident;
        }
        std::string GetType() const override{
            if(def == def_common)
                return "i32";
            
            // for lv9
            assert(arrayDimension != -1);
            std::vector<int32_t> lens;
            for(auto &&it : constExpArray)
                lens.push_back(std::stoi(it->Dump()));
            std::string arrayType = getArrayType(&lens, 0);
            return "*" + arrayType;
        }
};

class DeclAST : public BaseAST{
    public:
        enum {def_const, def_var} def;
        BaseASTPtr decl;

        std::string Dump() const override { 
            return decl->Dump(); 
        }
        int32_t CalValue() const override { 
            return decl->CalValue(); 
        }
        std::string DumpGlobal() const override { 
            return decl->DumpGlobal(); 
        }
};

class ConstDeclAST : public BaseAST{
    public:
        std::string bType;
        MulVecType constDefs;

        std::string Dump() const override { 
            for (auto&& it : constDefs)
                it->Dump();
            return "";
        }
        int32_t CalValue() const override { 
            for (auto&& it : constDefs)
                it->CalValue();
            return 0;
        }
        std::string DumpGlobal() const override { 
            for (auto&& it : constDefs)
                it->DumpGlobal(); 
            return "";
        }
};

class ConstDefAST : public BaseAST{
    public:
        std::string ident;
        BaseASTPtr constInitVal;
        MulVecType constExpArray;
        bool isArray = 0;

        std::string Dump() const override { 
            if(!constExpArray.empty()){// for lv9
                DumpArray();
                return "";
            }
            assert(constInitVal->GetIsArray() == 0);
            symTabs.back()[ident] = std::stoi(constInitVal->Dump());    
            // Note: the following code results in error
            // auto curSymTab = symTabs.back();
            // curSymTab[ident] = std::stoi(constInitVal->Dump());    
            return "";
        }

        int32_t CalValue() const override { 
            if(!constInitVal->GetIsArray()){
                symTabs.back()[ident] = std::stoi(constInitVal->Dump());    
                return 0;
            }
            // for lv9
            std::vector<int32_t> lens;
            for (auto&& it : constExpArray){
                lens.push_back(std::stoi(it->Dump()));
            }
            constArrayNum = 0;
            ArrayInitVals.clear();
            ArrayInitVals = constInitVal->GetArrayInitVals(lens);
            constArrayNum = 0;

            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            ArrayDimension[symName] = lens.size();
            isArrayGlobal[symName] = 1;

            std::cout << "\t" << symName << " = alloc " << getArrayType(&lens, 0) << "\n";
            printArrayInit(lens, 0, 0);
            std::cout << "\n";
            return 0;
        }
        std::string DumpGlobal() const override { 
            if(!constExpArray.empty()){
                assert(isArray);
                return DumpGlobalArray();
            }
            symTabs.back()[ident] = std::stoi(constInitVal->Dump());  
            return "";
        }
        std::string DumpGlobalArray() const override { 
            std::vector<int32_t> lens;
            for (auto&& it : constExpArray){
                std::string lastIR = it->Dump();
                // std::cout << "// in ConstDefAST DumpArray()\n";
                lens.push_back(std::stoi(lastIR));
            }
            constArrayNum = 0;
            ArrayInitVals.clear();
            ArrayInitVals = constInitVal->GetArrayInitVals(lens);
            constArrayNum = 0;

            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            ArrayDimension[symName] = lens.size();
            isArrayGlobal[symName] = 1;

            std::cout << "global " << symName << " = alloc " << getArrayType(&lens, 0) << ", ";
            printArrayInit(lens, 0, 0);
            std::cout << "\n";

            return "";
        }
                
        std::string DumpArray() const override {
            std::vector<int32_t> lens;
            for (auto&& it : constExpArray){
                std::string lastIR = it->Dump();
                // std::cout << "// in ConstDefAST DumpArray()\n";
                lens.push_back(std::stoi(lastIR));
            }
            constArrayNum = 0;
            ArrayInitVals.clear();
            ArrayInitVals = constInitVal->GetArrayInitVals(lens);

            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            ArrayDimension[symName] = lens.size();
            isArrayGlobal[symName] = 1;

            std::cout << "\t" << symName << " = alloc " << getArrayType(&lens, 0) << "\n";

            constArrayNum = 0;
            for (int32_t i = 0; i < lens[0]; i++){
                std::string nextSymName = getNewSymName();
                // std::cout << "// ConstDef DumpArray: loop" << i << "\n";
                std::cout << "\t" << nextSymName << " = getelemptr " << symName << ", " << i << "\n";
                ArrayInit(lens, nextSymName, 1, 0);
            }
            return "";
        }
};

class ConstInitValAST : public BaseAST{
    public:
        std::string ident;
        BaseASTPtr subExp;
        MulVecType constInitVals;
        bool isArray = 0;

        std::string Dump() const override { 
            assert(isArray == 0);
            std::string retValue = std::to_string(subExp->CalValue());
            return retValue;
            assert(0);
            return "";
        }
        std::string GetIdent() const override{ 
            return ident; 
            // TODO: modify it after supporting list
        }
        int32_t CalValue() const override {
            return subExp->CalValue();
        }
        std::string DumpGlobal() const override { 
            return "";
        }
        std::string TypeOfAST() const override{
            return "ConstInitVal";
        }
        
        std::vector<int32_t> GetArrayInitVals(std::vector<int32_t> lens) const override{
            std::vector<int32_t> arrayInitVals;
            if (lens.size() == 1){
                for (auto&& it : constInitVals){
                    constArrayNum++;
                    arrayInitVals.push_back(it->CalValue());
                }
                for (int32_t i = arrayInitVals.size(); i < lens[0]; i++){
                    constArrayNum++;
                    arrayInitVals.push_back(0); 
                }
            }
            else{
                std::vector<int32_t> lensProduct = getProduct(lens);
                for (auto&& it : constInitVals){
                    if (!it->GetIsArray()){
                        arrayInitVals.push_back(it->CalValue());
                        constArrayNum++; 
                        continue;
                    }
                    // else
                    for (int32_t i = 1; i < lens.size(); i++){
                        if (constArrayNum % lensProduct[i] == 0){
                            std::vector<int32_t> tmp = std::vector<int32_t>(lens.begin() + i, lens.end());
                            std::vector<int32_t> subArrayInitVals = it->GetArrayInitVals(tmp);
                            arrayInitVals.insert(arrayInitVals.end(), subArrayInitVals.begin(), subArrayInitVals.end());
                            break;
                        }
                        // else if (i == lens.size() - 1)
                        //     assert(false);
                    }
                }
                for (int32_t i = arrayInitVals.size(); i < lensProduct[0]; i++){
                    arrayInitVals.push_back(0); 
                    constArrayNum++;
                }
            }
            return arrayInitVals;
        }
        
        bool GetIsArray() const override{
            return isArray;
        }
};

class VarDeclAST : public BaseAST{
    public:
        std::string bType;
        MulVecType varDefs;

        std::string Dump() const override{
            for (auto&& it : varDefs){
                it->Dump();
            }
            return "";
        }
        int32_t CalValue() const override {
            for (auto&& it : varDefs){
                it->CalValue();
            }
            return 0;
        }
        std::string DumpGlobal() const override { 
            for (auto&& it : varDefs)
                it->DumpGlobal(); 
            return "";
        }
};

class VarDefAST : public BaseAST{
    public:
        std::string ident;
        BaseASTPtr initVal;
        bool isInitialized = 0;
        MulVecType constExpArray;

        std::string Dump() const override { 
            if(!constExpArray.empty()) {// for lv9 Arrays
                DumpArray();
                return "";
            }
            if(symNameCount.count(ident) == 0){
                symNameCount[ident] = 0;
            }
            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            std::cout << "\t" << symName << " = alloc i32\n";
            symTabs.back()[ident] = symName;
            if (initVal){
                std::string lastIR = initVal->Dump();
                std::cout << "\tstore " << lastIR << ", " << symName << "\n";
            }
            return symName;
        }
        
        std::string DumpArray() const override {
            std::vector<int32_t> lens;
            for (auto&& it : constExpArray){
                std::string lastIR = it->Dump();
                lens.push_back(std::stoi(lastIR));
            }
            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            ArrayDimension[symName] = lens.size();
            isArrayGlobal[symName] = 1;

            std::cout << "\t" << symName << " = alloc " << getArrayType(&lens, 0) << "\n";
            if (isInitialized){
                varArrayNum = 0;
                ArrayInitVals.clear();
                ArrayInitVals = initVal->GetArrayInitVals(lens);
                varArrayNum = 0;
                for (int32_t i = 0; i < lens[0]; i++){
                    std::string nextSymName = getNewSymName();
                    // std::cout << "// VardefAST DumpArray: loop" << i << "\n";
                    std::cout << "\t" << nextSymName << " = getelemptr " << symName << ", " << i << "\n";
                    ArrayInit(lens, nextSymName, 1, 1);
                }
            }
            return "";
        }

        int32_t CalValue() const override { 
            if(constExpArray.empty()){
                // std::cout << "// in VarDefAST CalValue()\n";
                symTabs.back()[ident] = std::stoi(initVal->Dump());    
                return 0;
            }

            // for lv9
            std::vector<int32_t> lens;
            for (auto&& it : constExpArray){
                lens.push_back(std::stoi(it->Dump()));
            }
            varArrayNum = 0;
            ArrayInitVals.clear();
            ArrayInitVals = initVal->GetArrayInitVals(lens);
            varArrayNum = 0;

            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            ArrayDimension[symName] = lens.size();
            isArrayGlobal[symName] = 1;

            std::cout << "\t" << symName << " = alloc " << getArrayType(&lens, 0) << "\n";
            printArrayInit(lens, 0, 1);
            std::cout << "\n";
            return 0;
        }
        
        std::string DumpGlobal() const override { 
            if(!constExpArray.empty()){
                // for lv9
                return DumpGlobalArray();
            }

            if(!symNameCount[ident]){
                symNameCount[ident] = 0;
            }
            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            std::string initValIR;
            if (initVal){
                initValIR = initVal->Dump();
            }
            std::cout << "global " << symName << " = alloc i32, ";
            if(initVal && initValIR != "0")
                std::cout << initValIR << "\n";
            else std::cout << "zeroinit\n";
            return "";
        }

        std::string DumpGlobalArray() const override { 
            std::vector<int32_t> lens;
            for (auto&& it : constExpArray){
                std::string lastIR = it->Dump();
                lens.push_back(std::stoi(lastIR));
            }
            int32_t symNameTag = symNameCount[ident]++;
            std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
            symTabs.back()[ident] = symName;
            ArrayDimension[symName] = lens.size();
            isArrayGlobal[symName] = 1;

            std::cout << "global " << symName << " = alloc " << getArrayType(&lens, 0);
            if (isInitialized && !initVal->isEmptyInitArray){
                varArrayNum = 0;
                ArrayInitVals.clear();
                // Error init values here in case 5 (current hello.c).
                // GetArrayInitVals() in InitValAST is wrong, or printArrayInit(). Check the recurrence logic later.
                ArrayInitVals = initVal->GetArrayInitVals(lens);
                varArrayNum = 0;
                std::cout << ", ";
                printArrayInit(lens, 0, 1);
            }
            else std::cout << ", zeroinit"; 
            std::cout << "\n";
            return "";
        }
};

class InitValAST : public BaseAST{
    public:
        BaseASTPtr subExp;
        MulVecType initVals;
        bool isArray = 0;

        std::string Dump() const override {
            if(!isArray)
                return subExp->Dump();
            return "";
        }
        int32_t CalValue() const override {
            return subExp->CalValue();
        }
        // std::string GetIdent() const override{ 
        //     return ident; 
        //     // TODO: modify it after supporting list
        // }
        std::string TypeOfAST() const override{
            return "InitVal";
        }

        // 函数 dumpList() 递归地将数组的初始值补全
        std::vector<int32_t> GetArrayInitVals(std::vector<int32_t> lens) const override{
            std::vector<int32_t> arrayInitVals;
            if (lens.size() == 1){
                for (auto&& it : initVals){
                    // assert(!it->GetIsArray());
                    varArrayNum++;
                    arrayInitVals.push_back(it->CalValue());
                }
                // std::cout << "{size=" << lens[0] - arrayInitVals.size() << "}";
                for (int32_t i = arrayInitVals.size(); i < lens[0]; i++){
                    varArrayNum++;
                    arrayInitVals.push_back(0); 
                }
            }
            else{
                std::vector<int32_t> lensProduct = getProduct(lens);
                for (auto&& it : initVals){
                    if (!it->GetIsArray()){
                        arrayInitVals.push_back(it->CalValue());
                        varArrayNum++; 
                        continue;
                    }
                    // else
                    for (int32_t i = 1; i < lens.size(); i++){
                        if (varArrayNum % lensProduct[i] == 0){
                            std::vector<int32_t> tmp = std::vector<int32_t>(lens.begin() + i, lens.end());
                            std::vector<int32_t> subArrayInitVals = it->GetArrayInitVals(tmp);
                            arrayInitVals.insert(arrayInitVals.end(), subArrayInitVals.begin(), subArrayInitVals.end());
                            break;
                        }
                        // else if (i == lens.size() - 1)
                        //     assert(false);
                    }
                }
                // std::cout << "{size=" << lensProduct[0] - arrayInitVals.size() << "}";
                for (int32_t i = arrayInitVals.size(); i < lensProduct[0]; i++){
                    arrayInitVals.push_back(0); 
                    varArrayNum++;
                }
            }
            return arrayInitVals;
        }
        bool GetIsArray() const override{
            return isArray;
        } 
 };

class BlockAST : public BaseAST {
    public: 
        MulVecType blockItems;
        std::string func;
        std::string Dump() const override {
            SymTabType symTab;
            if (func != ""){
                FuncInfo funcInfo = FuncInfoList[func];
                for (int32_t i = 0; i < funcInfo.paramInfoList.size(); i++){
                    ParamInfo paramInfo = funcInfo.paramInfoList[i];
                    std::string symName = paramInfo.name;
                    symName[0] = '%';
                    symTab[paramInfo.ident] = symName;
                    isParam[symName] = 1;
                    std::cout << "\t" << symName << " = alloc ";
                    std::cout << paramInfo.type << "\n";
                    std::cout << "\tstore " << paramInfo.name << ", " << symName << "\n";
                }
            }
            symTabs.push_back(symTab);

            std::string blockEnd = "";
            for (auto&& it : blockItems){
                blockEnd = it->Dump();
                if (blockEnd == "ret" || blockEnd == "break" || blockEnd == "cont")
                    break;
            }
            symTabs.pop_back();
            return blockEnd;        
        }
};

class BlockItemAST : public BaseAST {
    public: 
        enum {def_decl, def_stmt} def;
        BaseASTPtr blockItem;

        std::string Dump() const override {
            return blockItem->Dump();
        }
};

class ComplexStmtAST : public BaseAST{
    public:
        enum {def_simple, def_openif, def_ifelse, def_while} def;
        BaseASTPtr subExp;
        BaseASTPtr subStmt;
        BaseASTPtr elseStmt;
        
        std::string Dump() const override{
            if(def == def_simple){
                return subExp->Dump();
            }
            else if (def == def_while){
                    std::string whileEntryTag = whileEntryString + std::to_string(whileNum);
                    std::string whileBodyTag = whileBodyString + std::to_string(whileNum);
                    std::string whileEndTag = whileEndString + std::to_string(whileNum);
                    whileTab.push_back(whileNum++);
                    std::cout << "\tjump " << whileEntryTag << "\n";
                    std::cout << whileEntryTag << ":\n";
                    std::string subExpIR = subExp->Dump();
                    std::cout << "\tbr " << subExpIR << ", " << whileBodyTag << ", " << whileEndTag << "\n";
                    std::cout << whileBodyTag << ":" << "\n";
                    std::string subStmtIR = subStmt->Dump();
                    if (subStmtIR != "ret" && subStmtIR != "break" && subStmtIR != "cont")
                        std::cout << "\tjump " << whileEntryTag << "\n";
                    std::cout << whileEndTag << ":\n";
                    whileTab.pop_back();        
                }
            else {
                std::string subExpIR = subExp->Dump();
                std::string thenTag = thenString + std::to_string(ifElseNum);
                std::string elseTag = elseString + std::to_string(ifElseNum);
                std::string endTag = endString + std::to_string(ifElseNum++);
                if(def == def_openif){
                    std::cout << "\tbr " << subExpIR << ", " << thenTag << ", " << endTag << "\n";
                    std::cout << thenTag << ":\n";
                    std::string subStmtIR = subStmt->Dump();
                    if (subStmtIR != "ret" && subStmtIR != "break" && subStmtIR != "cont")
                        std::cout << "\tjump " << endTag << "\n";
                    std::cout << endTag << ":\n";
                    // return subStmtIR;
                }
                else if(def == def_ifelse){
                    std::cout << "\tbr " << subExpIR << ", " << thenTag << ", " << elseTag << "\n";
                    std::cout << thenTag << ":\n";
                    std::string subStmtIR = subStmt->Dump();
                    if (subStmtIR != "ret" && subStmtIR != "break" && subStmtIR != "cont")
                        std::cout << "\tjump " << endTag << "\n";
                    std::cout << elseTag << ":\n";
                    std::string elseStmtIR = elseStmt->Dump();
                    if (elseStmtIR != "ret" && elseStmtIR != "break" && elseStmtIR != "cont")
                        std::cout << "\tjump " << endTag << "\n";
                    if ((subStmtIR == "ret" || subStmtIR == "break" || subStmtIR == "cont") 
                        && (elseStmtIR == "ret" || elseStmtIR == "break" || elseStmtIR == "cont"))
                        return "ret";
                    else std::cout << endTag << ":\n";
                }
            }
            return "";
        }
};

class StmtAST : public BaseAST {
    public: 
        enum {def_lval, def_ret, def_exp, def_block, def_break, def_continue, def_array} def;
        std::string lVal;
        BaseASTPtr subExp;
        MulVecType expArray;

        std::string Dump() const override {
            if(def == def_lval){
                std::string lastIR = subExp->Dump();
                SymTabEntry ste = symTabs_lookup(lVal);
                // assert(ste.index() == 1);
                std::cout << "\tstore " << lastIR << ", " << std::get<std::string>(ste) << "\n";
                return "";
            }
            else if(def == def_ret){
                if(subExp){
                    std::string lastIR = subExp->Dump();
                    std::cout << "\tret " << lastIR << "\n";    
                }
                else {
                    if(isRetInt)
                        std::cout << "\tret 0\n";
                    else std::cout << "\tret\n";
                }        
                return "ret";
            }
            else if (def == def_exp){
                if (subExp){
                    subExp->Dump();
                }
            }
            else if (def == def_block){
                return subExp->Dump();
            }
            else if (def == def_break){
                std::cout << "\tjump " << whileEndString << whileTab.back() << "\n";
                return "break";
            }
            else if (def == def_continue){
                std::cout << "\tjump " << whileEntryString << whileTab.back() << "\n";
                return "cont";
            }
            else if (def == def_array){
                SymTabEntry ste = symTabs_lookup(lVal);
                std::string symName = std::get<std::string>(ste);
                for (auto&& it : expArray){
                    std::string lastIR = it->Dump();
                    std::string newSymName = getNewSymName();
                    if (isParam[symName]){
                        std::cout << "\t" << newSymName << " = load " << symName << "\n";
                        std::string tmp = getNewSymName();
                        std::cout << "\t" << tmp << " = getptr " << newSymName << ", " << lastIR << "\n";
                        newSymName = tmp;
                    }
                    else{
                        // std::cout << "// StmtAST Dump()\n";
                        std::cout << "\t" << newSymName << " = getelemptr " << symName << ", " << lastIR << "\n";
                    }
                    symName = newSymName;
                }    
                std::string subexp = subExp->Dump();
                std::cout << "\tstore " << subexp << ", " << symName << "\n";        
                return "";
            }
            return "";
        }
};

class ConstExpAST : public BaseAST{
    public:
        BaseASTPtr subExp;

        std::string Dump() const override{
            return std::to_string(subExp->CalValue());
        }
        virtual int32_t CalValue() const override { 
            return subExp->CalValue(); 
        }
};

class ExpAST : public BaseAST{
    public:
        BaseASTPtr subExp;

        std::string Dump() const override{
            return subExp->Dump();
        }
        virtual int32_t CalValue() const override{
            return subExp->CalValue();
        }
};

class PrimaryExpAST : public BaseAST{
    public:
        enum {def_bracketexp, def_number, def_lval, def_array} def;
        BaseASTPtr subExp;
        std::string lVal;
        std::string arrayIdent; // for lv9
        MulVecType expArray; 
        int32_t number;

        std::string Dump() const override{
            std::string retValue = "";
            if(def == def_bracketexp){
                return subExp->Dump();
            }
            else if(def == def_number){
                return std::to_string(number);
            }
            else if(def == def_lval){
                SymTabEntry ste = symTabs_lookup(lVal);
                if (ste.index() == 0)
                    return std::to_string(std::get<int>(ste));
                std::string symName = std::get<std::string>(ste);
                if (isArrayGlobal[symName]){
                    retValue = getNewSymName();
                    std::cout << "// PrimaryExpAST Dump()\n";
                    std::cout << "\t" << retValue << " = getelemptr " << symName << ", 0\n";
                }
                else {
                    retValue = getNewSymName();
                    std::cout << "\t" << retValue << " = load " << symName << "\n";
                }
            }
            else if(def == def_array){ // for lv9
                SymTabEntry ste = symTabs_lookup(arrayIdent);
                std::string symName = std::get<std::string>(ste);
                bool isArrayLocal = isArrayGlobal[symName];
                bool isOldSymNameParam = isParam[symName];
                std::string newSymName;
                int32_t arrayDimension = ArrayDimension[symName];

                for (auto&& it : expArray){
                    retValue = it->Dump();
                    newSymName = getNewSymName();
                    if (isParam[symName]){
                        // std::cout << "// isParam[symName] 1\n";
                        std::string nextNewSymNAme = getNewSymName();
                        std::cout << "\t" << newSymName << " = load " << symName << "\n";
                        std::cout << "\t" << nextNewSymNAme << " = getptr " << newSymName << ", " << retValue << "\n";
                        newSymName = nextNewSymNAme;
                        symName = newSymName;
                    }
                    else{
                        // std::cout << "// not isParam[symName] 1\n";
                        std::cout << "\t" << newSymName << " = getelemptr " << symName << ", " << retValue << "\n";
                        symName = newSymName;
                    }
                }
                if (expArray.size() == arrayDimension){
                    retValue = getNewSymName();
                    std::cout << "\t" << retValue << " = load " << symName << "\n";
                }
                else if (isArrayLocal || isOldSymNameParam){
                    retValue = getNewSymName();
                    // std::cout << "// isArrayLocal || isOldSymNameParam 1\n";
                    std::cout << "\t" << retValue << " = getelemptr " << symName << ", 0\n";
                }
                else retValue = newSymName;
            }
            return retValue;
        }

        virtual int32_t CalValue() const override{
            assert(def != def_array);
            if (def == def_bracketexp){
                return subExp->CalValue();
            }
            else if (def == def_number){
                return number;
            }
            else if(def == def_lval){
                SymTabEntry ste = symTabs_lookup(lVal);
                assert(ste.index() == 0);
                return std::get<int32_t>(ste);        
            }
            return 0;  
        }
};

class UnaryExpAST : public BaseAST{
    public:
        enum {def_primaryexp, def_unaryexp, def_func} def;
        std::string op;
        std::string ident;
        BaseASTPtr subExp;
        MulVecType funcRParams;
        
        std::string Dump() const override{
            if(def == def_primaryexp){
                return subExp->Dump();
            }
            else if(def == def_unaryexp){
                std::string subexp = subExp->Dump();
                if (op == "+") {
                    return subexp;
                }
                else if (op == "-"){
                    std::cout << "\t%" << varNum << " = sub 0, " << subexp << "\n";
                }
                else if (op == "!"){
                    std::cout << "\t%" << varNum << " = eq " << subexp << ", 0\n";
                }
                return "%" + std::to_string(varNum++);
            }
            else if(def == def_func){
                // Generate codes to get parameters
                FuncInfo funcInfo = FuncInfoList[ident];
                std::string lastIR = "";
                std::vector<std::string> params;
                for (auto&& it : funcRParams)
                    params.push_back(it->Dump());
                std::cout << "\t";
                if (funcInfo.retType == "int"){
                    lastIR = getNewSymName();
                    std::cout << lastIR << " = ";
                }
                std::cout << "call @" << ident << "(";

                // Print parameters
                for (int32_t i = 0; i < params.size(); i++){
                    if(i != params.size() - 1)
                        std::cout << params[i] << ", "; 
                    else std::cout << params[i];
                }
                std::cout << ")\n";
                return lastIR;        
            }
            else assert(false);
            return "";
        }

        virtual int32_t CalValue() const override{
            if (def == def_primaryexp){
                return subExp->CalValue();
            }
            else if (def == def_unaryexp){
                int32_t subvalue = subExp->CalValue();
                if (op == "+"){
                    return subvalue;
                }
                else if (op == "-"){
                    return -subvalue;
                }   
                else if (op == "!"){
                    return !subvalue;
                }           
                else assert(false);
            }
            return 0;  
        }
};

class MulExpAST : public BaseAST{
    public:
        std::string op;
        BaseASTPtr mulExp;
        BaseASTPtr subExp;
        
        std::string Dump() const override{
            if (op == ""){ 
                // MulExp := UnaryExp
                return subExp->Dump();
            }
            else{
                // MulExp := MulExp MulOp UnaryExp
                std::string mulexp = mulExp->Dump();
                std::string subexp = subExp->Dump();
                if (op == "*") {
                    std::cout << "\t%" << varNum << " = mul " << mulexp << ", " <<
                        subexp << "\n";
                }
                else if (op == "/"){
                    std::cout << "\t%" << varNum << " = div " << mulexp << ", " <<
                        subexp << "\n";
                }
                else if (op == "%"){
                    std::cout << "\t%" << varNum << " = mod " << mulexp << ", " <<
                        subexp << "\n";
                }
                else assert(false);
                return "%" + std::to_string(varNum++);
            }
            return "";
        }

        virtual int32_t CalValue() const override{
            if (op == ""){
                return subExp->CalValue();
            }
            else if (op == "*"){
                return mulExp->CalValue() * subExp->CalValue();
            }
            else if (op == "/"){
                return mulExp->CalValue() / subExp->CalValue();
            }   
            else if (op == "%"){
                return mulExp->CalValue() % subExp->CalValue();
            }           
            else assert(false);
            return 0;  
        }
};

class AddExpAST : public BaseAST{
    public:
        std::string op;
        BaseASTPtr addExp;
        BaseASTPtr subExp;
        
        std::string Dump() const override
        {
            if (op == ""){ 
                // AddExp := MulExp
                return subExp->Dump();
            }
            else{
                // AddExp := AddExp AddOp MulExp
                std::string addexp = addExp->Dump();
                std::string subexp = subExp->Dump();
                if (op == "+") {
                    std::cout << "\t%" << varNum << " = add " << addexp << ", " <<
                        subexp << "\n";
                }
                else if (op == "-"){
                    std::cout << "\t%" << varNum << " = sub " << addexp << ", " <<
                        subexp << "\n";
                }
                else assert(false);
                return "%" + std::to_string(varNum++);
            }
            return "";
        }

        virtual int32_t CalValue() const override{
            if (op == ""){
                return subExp->CalValue();
            }
            else if (op == "+"){
                return addExp->CalValue() + subExp->CalValue();
            }
            else if (op == "-"){
                return addExp->CalValue() - subExp->CalValue();
            }        
            else assert(false);
            return 0;  
        }
};

class RelExpAST : public BaseAST{
    public:
        std::string op;
        BaseASTPtr relExp;
        BaseASTPtr subExp;

        std::string Dump() const override{
            if (op == ""){ 
                // RelExp := AddExp
                return subExp->Dump();
            }
            else{
                // RelExp := RelExp RELOP AddExp
                std::string relexp = relExp->Dump();
                std::string subexp = subExp->Dump();
                if (op == "<") {
                    std::cout << "\t%" << varNum << " = lt " << relexp << ", " 
                            << subexp << "\n";
                }
                else if (op == ">"){
                    std::cout << "\t%" << varNum << " = gt " << relexp << ", "
                            << subexp << "\n";
                }
                else if (op == "<="){
                    std::cout << "\t%" << varNum << " = le " << relexp << ", "
                            << subexp << "\n";
                }            
                else if (op == ">="){
                    std::cout << "\t%" << varNum << " = ge " << relexp << ", "
                            << subexp << "\n";
                }            
                else assert(false);
                return "%" + std::to_string(varNum++);
            }
            return "";
        }

        virtual int32_t CalValue() const override{
            if (op == ""){
                return subExp->CalValue();
            }
            else if (op == ">"){
                return relExp->CalValue() > subExp->CalValue();
            }
            else if (op == ">="){
                return relExp->CalValue() >= subExp->CalValue();
            }
            else if (op == "<"){
                return relExp->CalValue() < subExp->CalValue();
            }
            else if (op == ">="){
                return relExp->CalValue() <= subExp->CalValue();
            }
            else assert(false);
            return 0;  
        }
};

class EqExpAST : public BaseAST{
    public:
        std::string op;
        BaseASTPtr eqExp;
        BaseASTPtr subExp;

        std::string Dump() const override{
            if (op == ""){ 
                // EqExp := RelExp
                return subExp->Dump();
            }
            else{
                // EqExp := EqExp EQOP RelExp
                std::string eqexp = eqExp->Dump();
                std::string subexp = subExp->Dump();
                if (op == "==") {
                    std::cout << "\t%" << varNum << " = eq " << eqexp << ", " 
                            << subexp << "\n";
                }
                else if (op == "!="){
                    std::cout << "\t%" << varNum << " = ne " << eqexp << ", "
                            << subexp << "\n";
                }       
                else assert(false);
                return "%" + std::to_string(varNum++);
            }
            return "";
        }
        
        virtual int32_t CalValue() const override{
            if (op == ""){
                return subExp->CalValue();
            }
            else if (op == "=="){
                return eqExp->CalValue() == subExp->CalValue();
            }
            else if (op == "!="){
                return eqExp->CalValue() != subExp->CalValue();
            }
            else assert(false);
            return 0;  
        }
};

class LAndExpAST : public BaseAST{
    public:
        std::string op;
        BaseASTPtr lAndExp;
        BaseASTPtr subExp;

        std::string Dump() const override{
            if (op == ""){ 
                // LAndExp := EqExp
                return subExp->Dump();
            }
            
            // LAndExp := LAndExp LANDOP EqExp
            assert(op == "&&");
            std::string lhs = lAndExp->Dump();
            std::string thenTag = thenString + std::to_string(ifElseNum);
            std::string elseTag = elseString + std::to_string(ifElseNum);
            std::string endTag = endString + std::to_string(ifElseNum++);
            std::string tmpSymName1 = getNewSymName();
            std::cout << "\t" << tmpSymName1 << " = alloc i32\n";
            std::cout << "\tbr " << lhs << ", " << thenTag << ", " << elseTag << "\n";
            std::cout << thenTag << ":\n";
            std::string tmpSymName2 = getNewSymName();
            std::string rhs = subExp->Dump();
            std::cout << "\t" << tmpSymName2 << " = ne " << rhs << ", 0\n";
            std::cout << "\tstore " << tmpSymName2 << ", " << tmpSymName1 << "\n";
            std::cout << "\tjump " << endTag << "\n";
            std::cout << elseTag << ":\n";
            std::cout << "\tstore 0, " << tmpSymName1 << "\n";
            std::cout << "\tjump " << endTag << "\n";
            std::cout << endTag << ":\n";
            std::string symName = getNewSymName();
            std::cout << "\t" << symName << " = load " << tmpSymName1 << "\n";
            return symName;
        }

        virtual int32_t CalValue() const override{
            if (op == ""){
                return subExp->CalValue();
            }
            else if (op == "||"){
                return lAndExp->CalValue() && subExp->CalValue();
            }
            else assert(false);
            return 0;  
        }
};

class LOrExpAST : public BaseAST{
    public:
        std::string op;
        BaseASTPtr lOrExp;
        BaseASTPtr subExp;

        std::string Dump() const override{
            if (op == ""){ 
                // LOrExp := LAndExp
                return subExp->Dump();
            }
            
            // LOrExp := LOrExp LOROP LAndExp
            assert(op == "||");
            std::string lhs = lOrExp->Dump();
            std::string thenTag = thenString + std::to_string(ifElseNum);
            std::string elseTag = elseString + std::to_string(ifElseNum);
            std::string endTag = endString + std::to_string(ifElseNum++);
            std::string tmpSymName1 = getNewSymName();
            std::cout << "\t" << tmpSymName1 << " = alloc i32\n";
            std::cout << "\tbr " << lhs << ", " << thenTag << ", " << elseTag << "\n";
            std::cout << thenTag << ":\n";
            std::cout << "\tstore 1, " << tmpSymName1 << "\n";
            std::cout << "\tjump " << endTag << "\n";
            std::cout << elseTag << ":\n";
            std::string tmpSymName2 = getNewSymName();
            std::string rhs = subExp->Dump();
            std::cout << "\t" << tmpSymName2 << " = ne " << rhs << ", 0\n";
            std::cout << "\tstore " << tmpSymName2 << ", " << tmpSymName1 << "\n";
            std::cout << "\tjump " << endTag << "\n";
            std::cout << endTag << ":\n";
            std::string symName = getNewSymName();
            std::cout << "\t" << symName << " = load " << tmpSymName1 << "\n";
            return symName;
        }

        virtual int32_t CalValue() const override{
            if (op == ""){
                return subExp->CalValue();
            }
            else if (op == "||"){
                return lOrExp->CalValue() || subExp->CalValue();
            }
            else assert(false);
            return 1;  
        }
};

inline SymTabEntry symTabs_lookup(std::string lval){
    for (auto it = symTabs.rbegin(); it != symTabs.rend(); it++){
        if (it->count(lval))
            return (*it)[lval];    
    }
    return 0;
} 

static std::string getNewSymName(){
    return "%" + std::to_string(varNum++);
}

static void libFuncDecl(){
    std::cout << "decl @getint(): i32\n";
    std::cout << "decl @getch(): i32\n";
    std::cout << "decl @getarray(*i32): i32\n";
    std::cout << "decl @putint(i32)\n";
    std::cout << "decl @putch(i32)\n";
    std::cout << "decl @putarray(i32, *i32)\n";
    std::cout << "decl @starttime()\n";
    std::cout << "decl @stoptime()\n";
    std::cout << "\n";
    
    FuncInfo funcInfo;
    funcInfo.retType = "int";
    FuncInfoList["getint"] = funcInfo;
    FuncInfoList["getch"] = funcInfo;
    FuncInfoList["getarray"] = funcInfo;
    funcInfo.retType = "void";
    FuncInfoList["putint"] = funcInfo;
    FuncInfoList["putch"] = funcInfo;
    FuncInfoList["putarray"] = funcInfo;
    FuncInfoList["starttime"] = funcInfo;
    FuncInfoList["stoptime"] = funcInfo;
}

static std::string getArrayType(const std::vector<int32_t> *lens, int32_t d){
    if (d == lens->size()){
        return "i32";
    }
    return "[" + getArrayType(lens, d + 1) + ", " + std::to_string((*lens)[d]) + "]";
}

static int32_t getArraySize(std::vector<int32_t> offset, int32_t curNum){
    int32_t retValue = 1;
    for(int32_t i = 0;i < offset.size();i++){
        if(curNum % offset[i] == 0)
            retValue = offset[i];
        else
            break;
    }
    return retValue;
}

// isVarArray = 1 -> varArrayNum, isVarArray = 0 ->constArrayNum
static std::string ArrayInit(std::vector<int32_t> lens, std::string lastSymName, int32_t d, int32_t isVarDef) {
    if (d >= lens.size()){
        int32_t thisVal;
        if(isVarDef){
            thisVal = ArrayInitVals[varArrayNum++];
        }
        else thisVal = ArrayInitVals[constArrayNum++];
        std::cout << "\tstore " << thisVal << ", " << lastSymName << "\n";
        return "";
    }
    for (int32_t i = 0; i < lens[d]; i++){
        std::string symName = getNewSymName();
        // std::cout << "// ArrayInit: loop" << i << "\n";
        std::cout << "\t" << symName << " = getelemptr " << lastSymName << ", " << i << "\n";
        ArrayInit(lens, symName, d + 1, isVarDef);
    }
    return "";
}

// isVarArray = 1 -> varArrayNum, isVarArray = 0 ->constArrayNum
static void printArrayInit(std::vector<int32_t> lens, int32_t d, int32_t isVarDef){
    if (d >= lens.size()){
        if(isVarDef){
            std::cout << ArrayInitVals[varArrayNum++]; 
        }
        else 
            std::cout << ArrayInitVals[constArrayNum++]; 
        return;
    }
    std::cout << "{";
    for (int32_t i = 0; i < lens[d]; i++){
        printArrayInit(lens, d + 1, isVarDef);
        if (i < lens[d] - 1)
            std::cout << ", ";
    }
    std::cout << "}";
}

static std::vector<int32_t> getProduct(std::vector<int32_t> vec){
    std::vector<int32_t> ans = vec;
    for (int32_t i = ans.size() - 1; i > 0; i--)
        ans[i - 1] *= ans[i];
    return ans;
}
