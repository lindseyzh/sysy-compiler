#pragma once
#include"AST_util.hpp"

// Variants for in AST generation.
// Note: Linking error occurs if I put them in AST_util.hpp, even with #pragma once.

static const std::string thenString = "\%then_";
static const std::string elseString = "\%else_";
static const std::string endString = "\%end_";
static const std::string whileEntryString = "\%while_entry_";
static const std::string whileBodyString = "\%while_body_";
static const std::string whileEndString = "\%while_end_";

static int32_t varNum = 0;
static int32_t ifElseNum = 0;
static int32_t whileNum = 0;
static bool isRetInt = 1;

static std::vector<SymTabType> symTabs;
static std::unordered_map<std::string, bool> isParam;
static std::unordered_map<std::string, int32_t> symNameCount; // Map ident to its count
static std::vector<int32_t> whileTab; // Recording current while layers
static std::unordered_map<std::string, FuncInfo> FuncInfoList;

// Base class for all ASTs
class BaseAST {
    public:
    virtual ~BaseAST() = default;

    virtual std::string DumpIR() const{ assert(0); return 0; }
    virtual int32_t CalValue() const{ assert(0); return -1; }
    virtual std::string GetIdent() const{ assert(0); return ""; }
    virtual std::string GetType() const{ assert(0); return ""; }
};

class CompUnitAST : public BaseAST {
    public:
        MulVecType funcDefs;
        std::string DumpIR() const override {
            libFuncDecl(); // Generate declaration for library functions
            SymTabType globalSymTab;
            symTabs.push_back(globalSymTab);
            for (auto&& funcDef : funcDefs)
                funcDef->DumpIR();            
            symTabs.pop_back();
            return "";
        }
};

class FuncDefAST : public BaseAST {
    public:
    std::string funcType;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    MulVecType funcFParams;

    std::string DumpIR() const override {
        std::string funcName = "@" + ident;
        // assert(!symbol_tables[0].count(ident));
        // assert(!function_table.count(ident));
        FuncInfo funcInfo;
        funcInfo.retType = funcType;
        std::cout << "fun @" << ident << "(";

        // for (int32_t i = 0; i < 0; i++){
        for (int32_t i = 0; i < funcFParams.size(); i++){
            ParamInfo paramInfo;
            paramInfo.ident = funcFParams[i]->GetIdent();
            paramInfo.name = funcFParams[i]->DumpIR();
            paramInfo.type = funcFParams[i]->GetType();
            // TODO: continue to finish the function.
            // if (paramInfo.type != "i32"){
            //     std::string tmp = names.back();
            //     tmp[0] = '%';
            //     list_dim[tmp] = funcFParams[i]->get_dim();
            // }
            funcInfo.paramInfoList.push_back(paramInfo);
            std::cout << ": " << paramInfo.type;
            if (i != funcFParams.size() - 1)
                std::cout << ", ";
        }
        std::cout << ")";
        FuncInfoList[ident] = funcInfo;
        // function_param_idents[ident] = move(idents);
        // function_param_names[ident] = move(names);
        // function_param_types[ident] = move(types);
        if(funcType == "int")
            std::cout << ": i32";
        else assert(funcType == "void");
        std::cout << " {" << "\n";
        std::cout << "%entry_" << ident << ":\n";

        isRetInt = (funcType == "int");
            
        std::string blockEnd = block->DumpIR();
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
        std::string bType;
        std::string ident;
        std::string DumpIR() const override{
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
            return "i32";
        }
};

class DeclAST : public BaseAST
{
    public:
        enum {def_const, def_var} def;
        std::unique_ptr<BaseAST> decl;

        std::string DumpIR() const override { 
            return decl->DumpIR(); 
        }
        int32_t CalValue() const override { 
            return decl->CalValue(); 
        }
};

class ConstDeclAST : public BaseAST
{
    public:
        std::string bType;
        MulVecType constDefs;

        std::string DumpIR() const override { 
            for (auto&& it : constDefs)
                it->DumpIR();
            return "";
        }
        int32_t CalValue() const override { 
            for (auto&& it : constDefs)
                it->CalValue();
            return 0;
        }
};

class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> constInitVal;

    std::string DumpIR() const override { 
        symTabs.back()[ident] = std::stoi(constInitVal->DumpIR());    
        // Note: the following code results in error
        // auto curSymTab = symTabs.back();
        // curSymTab[ident] = std::stoi(constInitVal->DumpIR());    
        return "";
    }
    int32_t CalValue() const override { 
        symTabs.back()[ident] = std::stoi(constInitVal->DumpIR());    
        return 0;
    }
};

class ConstInitValAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override { 
        std::string retValue = std::to_string(subExp->CalValue());
        return retValue;
    }
    std::string GetIdent() const override{ 
        return ident; 
        // TODO: modify it after supporting list
    }
};

class VarDeclAST : public BaseAST
{
public:
    std::string bType;
    MulVecType varDefs;

    std::string DumpIR() const override{
        for (auto&& it : varDefs){
            it->DumpIR();
        }
        return "";
    }
    int32_t CalValue() const override {
        for (auto&& it : varDefs){
            it->CalValue();
        }
        return 0;
    }
};

class VarDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> initVal;

    std::string DumpIR() const override { 
        if(symNameCount.count(ident) == 0){
            symNameCount[ident] = 0;
        }
        int32_t symNameTag = symNameCount[ident]++;
        std::string symName = "@" + ident + "_" + std::to_string(symNameTag);
        std::cout << "\t" << symName << " = alloc i32\n";
        symTabs.back()[ident] = symName;
        if (initVal){
            std::string lastIR = initVal->DumpIR();
            std::cout << "\tstore " << lastIR << ", " << symName << "\n";
        }
        return symName;
    }
    int32_t CalValue() const override { 
        symTabs.back()[ident] = std::stoi(initVal->DumpIR());    
        return 0;
    }
};

class InitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override {
        return subExp->DumpIR();
    }
    int32_t CalValue() const override {
        return subExp->CalValue();
    }
    // std::string GetIdent() const override{ 
    //     return ident; 
    //     // TODO: modify it after supporting list
    // }
};

class BlockAST : public BaseAST {
    public: 
        MulVecType blockItems;
        std::string func;
        std::string DumpIR() const override {
            SymTabType symTab;
            if (func != ""){
                FuncInfo funcInfo = FuncInfoList[func];
                for (int32_t i = 0; i < funcInfo.paramInfoList.size(); i++){
                    ParamInfo paramInfo = funcInfo.paramInfoList[i];
                    std::string symName = paramInfo.name;
                    symName[0] = '%';
                    symTab[paramInfo.ident] = symName;
                    isParam[symName] = 1;
                    std::cout << '\t' << symName << " = alloc ";
                    std::cout << paramInfo.type << "\n";
                    std::cout << "\tstore " << paramInfo.name << ", " << symName << "\n";
                }
            }
            symTabs.push_back(symTab);

            std::string blockEnd = "";
            for (auto&& it : blockItems){
                blockEnd = it->DumpIR();
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
        std::unique_ptr<BaseAST> blockItem;

    std::string DumpIR() const override {
        return blockItem->DumpIR();
    }
};

class ComplexStmtAST : public BaseAST
{
public:
    enum {def_simple, def_openif, def_ifelse, def_while} def;
    std::unique_ptr<BaseAST> subExp;
    std::unique_ptr<BaseAST> subStmt;
    std::unique_ptr<BaseAST> elseStmt;
    
    std::string DumpIR() const override{
        if(def == def_simple){
            return subExp->DumpIR();
        }
        else if (def == def_while){
                std::string whileEntryTag = whileEntryString + std::to_string(whileNum);
                std::string whileBodyTag = whileBodyString + std::to_string(whileNum);
                std::string whileEndTag = whileEndString + std::to_string(whileNum);
                whileTab.push_back(whileNum++);
                std::cout << "\tjump " << whileEntryTag << "\n";
                std::cout << whileEntryTag << ":\n";
                std::string subExpIR = subExp->DumpIR();
                std::cout << "\tbr " << subExpIR << ", " << whileBodyTag << ", " << whileEndTag << "\n";
                std::cout << whileBodyTag << ":" << "\n";
                std::string subStmtIR = subStmt->DumpIR();
                if (subStmtIR != "ret" && subStmtIR != "break" && subStmtIR != "cont")
                    std::cout << "\tjump " << whileEntryTag << "\n";
                std::cout << whileEndTag << ":\n";
                whileTab.pop_back();        
            }
        else {
            std::string subExpIR = subExp->DumpIR();
            std::string thenTag = thenString + std::to_string(ifElseNum);
            std::string elseTag = elseString + std::to_string(ifElseNum);
            std::string endTag = endString + std::to_string(ifElseNum++);
            if(def == def_openif){
                std::cout << "\tbr " << subExpIR << ", " << thenTag << ", " << endTag << "\n";
                std::cout << thenTag << ":\n";
                std::string subStmtIR = subStmt->DumpIR();
                if (subStmtIR != "ret" && subStmtIR != "break" && subStmtIR != "cont")
                    std::cout << "\tjump " << endTag << "\n";
                std::cout << endTag << ":\n";
                // return subStmtIR;
            }
            else if(def == def_ifelse){
                std::cout << "\tbr " << subExpIR << ", " << thenTag << ", " << elseTag << "\n";
                std::cout << thenTag << ":\n";
                std::string subStmtIR = subStmt->DumpIR();
                if (subStmtIR != "ret" && subStmtIR != "break" && subStmtIR != "cont")
                    std::cout << "\tjump " << endTag << "\n";
                std::cout << elseTag << ":\n";
                std::string elseStmtIR = elseStmt->DumpIR();
                if (elseStmtIR != "ret" && elseStmtIR != "break" && elseStmtIR != "cont")
                    std::cout << "\tjump " << endTag << "\n";
                if ((subStmtIR == "ret" || subStmtIR == "break" || subStmtIR == "cont") 
                    && (elseStmtIR == "ret" || elseStmtIR == "break" || elseStmtIR == "cont"))
                    return "ret";
                else std::cout << endTag << ":\n";
                // return subStmtIR;
            }
            // else assert(false);
        }
        return "";
    }
};

class StmtAST : public BaseAST {
    public: 
        enum {def_lval, def_ret, def_exp, def_block, def_break, def_continue} def;
        std::string lVal;
        std::unique_ptr<BaseAST> subExp;

        std::string DumpIR() const override {
            if(def == def_lval){
                std::string lastIR = subExp->DumpIR();
                SymTabEntry ste = symTabs_lookup(lVal);
                // assert(ste.index() == 1);
                std::cout << "\tstore " << lastIR << ", " << std::get<std::string>(ste) << "\n";
            }
            else if(def == def_ret){
                if(subExp){
                    std::string lastIR = subExp->DumpIR();
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
                    subExp->DumpIR();
                }
            }
            else if (def == def_block){
                return subExp->DumpIR();
            }
            else if (def == def_break){
                // assert(!whileTab.empty());
                std::cout << "\tjump " << whileEndString << whileTab.back() << "\n";
                return "break";
            }
            else if (def == def_continue){
                // assert(!whileTab.empty());
                std::cout << "\tjump " << whileEntryString << whileTab.back() << "\n";
                return "cont";
            }
            // else assert(false);
            return "";
        }
};

class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override{
        return std::to_string(subExp->CalValue());
    }
    virtual int32_t CalValue() const override { 
        return subExp->CalValue(); 
    }
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override{
        return subExp->DumpIR();
    }
    virtual int32_t CalValue() const override{
        return subExp->CalValue();
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    enum {def_bracketexp, def_number, def_lval} def;
    std::unique_ptr<BaseAST> subExp;
    std::string lVal;
    // std::vector<std::unique_ptr<BaseAST>> exp_list;
    int32_t number;

    std::string DumpIR() const override{
        std::string retValue = "";
        if (def == def_bracketexp){
            return subExp->DumpIR();
        }
        else if (def == def_number){
            return std::to_string(number);
        }
        else if (def == def_lval){
            SymTabEntry ste = symTabs_lookup(lVal);
            if (ste.index() == 0)
                retValue = std::to_string(std::get<int>(ste));
            // else if (is_list[std::get<std::string>(ste)])
            // {
            //     retValue = "%" + std::to_string(varNum++);
            //     std::cout << '\t' << retValue << " = getelemptr " <<
            //         std::get<std::string>(ste) << ", 0" << std::endl;
            // }
            else {
                retValue = "%" + std::to_string(varNum++);
                std::cout << '\t' << retValue << " = load " <<
                    std::get<std::string>(ste) << "\n";
            }
        }
        return retValue;
    }
    // TODO: Finish CalValue(); Add support for lval
    virtual int32_t CalValue() const override{
        if (def == def_bracketexp){
            return subExp->CalValue();
        }
        else if (def == def_number){
            return number;
        }
        else if(def == def_lval){
            SymTabEntry ste = symTabs_lookup(lVal);
            assert(ste.index() == 0);
            return std::get<int>(ste);        
        }
        else assert(false);
        return 0;  
    }

};

class UnaryExpAST : public BaseAST
{
public:
    enum {def_primaryexp, def_unaryexp, def_func} def;
    std::string op;
    std::string ident;
    std::unique_ptr<BaseAST> subExp;
    MulVecType funcRParams;
    
    std::string DumpIR() const override{
        if(def == def_primaryexp){
            return subExp->DumpIR();
        }
        else if(def == def_unaryexp){
            std::string subexp = subExp->DumpIR();
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
            // Generate codes to calculate parameters
            FuncInfo funcInfo = FuncInfoList[ident];
            std::string lastIR = "";
            std::vector<std::string> params;
            for (auto&& it : funcRParams)
                params.push_back(it->DumpIR());
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

class MulExpAST : public BaseAST
{
public:
    std::string op;
    std::unique_ptr<BaseAST> mulExp;
    std::unique_ptr<BaseAST> subExp;
    
    std::string DumpIR() const override
    {
        if (op == ""){ 
            // MulExp := UnaryExp
            return subExp->DumpIR();
        }
        else{
            // MulExp := MulExp MulOp UnaryExp
            std::string mulexp = mulExp->DumpIR();
            std::string subexp = subExp->DumpIR();
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

class AddExpAST : public BaseAST
{
public:
    std::string op;
    std::unique_ptr<BaseAST> addExp;
    std::unique_ptr<BaseAST> subExp;
    
    std::string DumpIR() const override
    {
        if (op == ""){ 
            // AddExp := MulExp
            return subExp->DumpIR();
        }
        else{
            // AddExp := AddExp AddOp MulExp
            std::string addexp = addExp->DumpIR();
            std::string subexp = subExp->DumpIR();
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

class RelExpAST : public BaseAST
{
public:
    std::string op;
    std::unique_ptr<BaseAST> relExp;
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override
    {
        if (op == ""){ 
            // RelExp := AddExp
            return subExp->DumpIR();
        }
        else{
            // RelExp := RelExp RELOP AddExp
            std::string relexp = relExp->DumpIR();
            std::string subexp = subExp->DumpIR();
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

class EqExpAST : public BaseAST
{
public:
    std::string op;
    std::unique_ptr<BaseAST> eqExp;
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override
    {
        if (op == ""){ 
            // EqExp := RelExp
            return subExp->DumpIR();
        }
        else{
            // EqExp := EqExp EQOP RelExp
            std::string eqexp = eqExp->DumpIR();
            std::string subexp = subExp->DumpIR();
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

class LAndExpAST : public BaseAST
{
public:
    std::string op;
    std::unique_ptr<BaseAST> lAndExp;
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override
    {
        if (op == ""){ 
            // LAndExp := EqExp
            return subExp->DumpIR();
        }
        
        // LAndExp := LAndExp LANDOP EqExp
        assert(op == "&&");
        std::string lhs = lAndExp->DumpIR();
        std::string thenTag = thenString + std::to_string(ifElseNum);
        std::string elseTag = elseString + std::to_string(ifElseNum);
        std::string endTag = endString + std::to_string(ifElseNum++);
        std::string tmpSymName1 = getNewSymName();
        std::cout << '\t' << tmpSymName1 << " = alloc i32\n";
        std::cout << "\tbr " << lhs << ", " << thenTag << ", " << elseTag << "\n";
        std::cout << thenTag << ":\n";
        std::string tmpSymName2 = getNewSymName();
        std::string rhs = subExp->DumpIR();
        std::cout << '\t' << tmpSymName2 << " = ne " << rhs << ", 0\n";
        std::cout << "\tstore " << tmpSymName2 << ", " << tmpSymName1 << "\n";
        std::cout << "\tjump " << endTag << "\n";
        std::cout << elseTag << ":\n";
        std::cout << "\tstore 0, " << tmpSymName1 << "\n";
        std::cout << "\tjump " << endTag << "\n";
        std::cout << endTag << ":\n";
        std::string symName = getNewSymName();
        std::cout << '\t' << symName << " = load " << tmpSymName1 << "\n";
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

class LOrExpAST : public BaseAST
{
public:
    std::string op;
    std::unique_ptr<BaseAST> lOrExp;
    std::unique_ptr<BaseAST> subExp;

    std::string DumpIR() const override{
        if (op == ""){ 
            // LOrExp := LAndExp
            return subExp->DumpIR();
        }
        
        // LOrExp := LOrExp LOROP LAndExp
        assert(op == "||");
        std::string lhs = lOrExp->DumpIR();
        std::string thenTag = thenString + std::to_string(ifElseNum);
        std::string elseTag = elseString + std::to_string(ifElseNum);
        std::string endTag = endString + std::to_string(ifElseNum++);
        std::string tmpSymName1 = getNewSymName();
        std::cout << '\t' << tmpSymName1 << " = alloc i32\n";
        std::cout << "\tbr " << lhs << ", " << thenTag << ", " << elseTag << "\n";
        std::cout << thenTag << ":\n";
        std::cout << "\tstore 1, " << tmpSymName1 << "\n";
        std::cout << "\tjump " << endTag << "\n";
        std::cout << elseTag << ":\n";
        std::string tmpSymName2 = getNewSymName();
        std::string rhs = subExp->DumpIR();
        std::cout << '\t' << tmpSymName2 << " = ne " << rhs << ", 0\n";
        std::cout << "\tstore " << tmpSymName2 << ", " << tmpSymName1 << "\n";
        std::cout << "\tjump " << endTag << "\n";
        std::cout << endTag << ":\n";
        std::string symName = getNewSymName();
        std::cout << '\t' << symName << " = load " << tmpSymName1 << "\n";
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

inline std::string getNewSymName(){
    return "%" + std::to_string(varNum++);
}

inline void libFuncDecl(){
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