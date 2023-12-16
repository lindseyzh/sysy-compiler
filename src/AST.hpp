#pragma once
#include<memory>
#include<string>
#include<iostream>
#include<cassert>
#include<vector>
#include<map>
#include<unordered_map>
#include<variant>

class BaseAST;
typedef std::variant<int, std::string> SymTabEntry;
typedef std::unordered_map<std::string, SymTabEntry> SymTabType;
typedef std::vector<std::unique_ptr<BaseAST>> MulVecType;

static int varNum = 0;
static std::vector<SymTabType> symTabs;

inline static SymTabEntry symTabs_lookup(std::string lval);

// Base class for all ASTs
class BaseAST {
    public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
    virtual int32_t CalValue() const {
        // NO Reached
        return -1;
    }
};

class CompUnitAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    std::string DumpIR() const override {
        SymTabType globalSymTab;
        symTabs.push_back(globalSymTab);
        func_def->DumpIR();
        symTabs.pop_back();
        return "";
    }
};

class FuncDefAST : public BaseAST {
    public:
    std::string func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST { " ;
        std::cout << "FuncTypeAST { " << func_type << " }";
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }" << std::endl;
    }

    std::string DumpIR() const override {
        std::cout << "fun @" << ident << "(): ";
        std::cout << "i32";
        std::cout << " {" << "\n";
        std::cout << "%entry:" << "\n";
        block->DumpIR();
        std::cout << "}" << std::endl;
        return "";
    }
};

class DeclAST : public BaseAST
{
    public:
        enum {def_const, def_var} def;
        std::unique_ptr<BaseAST> decl;
        void Dump() const override { 
            decl->Dump(); 
        }
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
        void Dump() const override { 
            for (auto&& it : constDefs)
                it->Dump();
        }
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
    void Dump() const override { 
        std::cout << "ConstDefAST{" << ident << "=";
        constInitVal->Dump();
        std::cout << "} ";    
    }
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
    void Dump() const override { 
        std::cout << subExp->CalValue();    
    }
    std::string DumpIR() const override { 
        std::string retValue = std::to_string(subExp->CalValue());
        return retValue;
    }
};

class VarDeclAST : public BaseAST
{
public:
    std::string bType;
    MulVecType varDefs;
    void Dump() const override{
        for(auto&& it : varDefs){
            it->Dump();
        }
    }
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
    void Dump() const override { 
        std::cout << "VarDefAST{" << ident << "=";
        initVal->Dump();
        std::cout << "} ";    
    }
    std::string DumpIR() const override { 
        std::string symName = "@" + ident;
        std::cout << "\t" << symName << " = alloc i32\n";
        symTabs.back()[ident] = symName;
        if (initVal){
            std::string lastIR = initVal->DumpIR();
            std::cout << "\tstore " << lastIR << ", " << symName << "\n";
        }
        return "";
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
    void Dump() const override {
        subExp->Dump();
    }
    std::string DumpIR() const override {
        return subExp->DumpIR();
    }
    int32_t CalValue() const override {
        return subExp->CalValue();
    }
};

class BlockAST : public BaseAST {
    public: 
        MulVecType blockItems;
        std::string func;
        void Dump() const override {
            std::cout << "BlockAST { ";
                for (auto&& it : blockItems)
                    it->Dump();
            std::cout << " }" << std::endl;
        }
        std::string DumpIR() const override {
            std::string lastIR = "";
            SymTabType symTab;
            symTabs.push_back(symTab);
            for (auto&& it : blockItems)
            {
                lastIR = it->DumpIR();
                if (lastIR == "ret" || lastIR == "break" || lastIR == "cont")
                    break;
            }
            symTabs.pop_back();
            return lastIR;        
        }
};

class BlockItemAST : public BaseAST {
    public: 
        enum {def_decl, def_stmt} def;
        std::unique_ptr<BaseAST> blockItem;
    void Dump() const override {
        std::cout << "BlockAST { ";
        blockItem->Dump();
        std::cout << " }" << std::endl;
    }
    std::string DumpIR() const override {
        blockItem->DumpIR();
        return "";
    }
};

class StmtAST : public BaseAST {
    public: 
        enum {def_lval, def_ret} def;
        std::string lVal;
        std::unique_ptr<BaseAST> subExp;
        void Dump() const override {
            if(def == def_lval){
                std::cout << "LVal { " << lVal << " = ";
                subExp->Dump();
                std::cout << " }";            
            }
            else if(def == def_ret){
                std::cout << "RETURN { ";
                subExp->Dump();
                std::cout << " }";
            }
        }

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
                    std::cout << "\tret\n";
                    // TODO: add functype. if functype == int, return 0;
                }        
            }

            return "";
        }
};

class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> subExp;
    void Dump() const override { 
        std::cout << subExp->CalValue(); 
    }
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
    void Dump() const override{
        std::cout << "ExpAST { ";
        subExp->Dump();
        std::cout << " } ";
    }
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
    int number;
    void Dump() const override
    {
        if (def == def_bracketexp) {
            subExp->Dump();
        }
        else if (def == def_number){
            std::cout << number;
        }
        else if (def == def_lval){
            std::cout << lVal;
        }
        // else if (type == PrimaryExpType::list)
        // {
        //     std::cout << lval;
        //     for (auto&& exp : exp_list)
        //     {
        //         std::cout << '['; exp->dump(); std::cout << ']';
        //     }
        // }
        // else assert(false);
    }

    // TODO: modify the dumpIR algorithm
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
    enum {def_primaryexp, def_unaryexp} def;
    std::string op;
    std::unique_ptr<BaseAST> subExp;
    // std::string ident;
    // std::vector<std::unique_ptr<BaseAST>> params;
    void Dump() const override
    {
        if (def == def_unaryexp){
            std::cout << op;
            subExp->Dump();
        }
        else{
            subExp->Dump();
        }
    }
    
    std::string DumpIR() const override
    {
        if (def == def_primaryexp){
            return subExp->DumpIR();
        }
        else if (def == def_unaryexp){
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
        // TODO: More unaryexp types
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
    void Dump() const override
    {
        if (op == ""){ 
            // MulExp := UnaryExp
            subExp->Dump();
        }
        else{
            // MulExp := MulExp MulOp UnaryExp
            mulExp->Dump();
            std::cout << op;
            subExp->Dump();
        }
    }
    
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
    void Dump() const override
    {
        if (op == ""){ 
            // AddExp := MulExp
            subExp->Dump();
        }
        else{
            // AddExp := AddExp AddOp MulExp
            addExp->Dump();
            std::cout << op;
            subExp->Dump();
        }
    }
    
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
    void Dump() const override
    {
        if (op == ""){ 
            // RelExp := AddExp
            subExp->Dump();
        }
        else{
            // RelExp := RelExp RELOP AddExp
            relExp->Dump();
            std::cout << op;
            subExp->Dump();
        }
    }
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
    void Dump() const override
    {
        if (op == ""){ 
            // EqExp := RelExp
            subExp->Dump();
        }
        else{
            // EqExp := EqExp EQOP RelExp
            eqExp->Dump();
            std::cout << op;
            subExp->Dump();
        }
    }
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
    void Dump() const override
    {
        if (op == ""){ 
            // LAndExp := EqExp
            subExp->Dump();
        }
        else{
            // LAndExp := LAndExp LANDOP EqExp
            lAndExp->Dump();
            std::cout << op;
            subExp->Dump();
        }
    }

    std::string DumpIR() const override
    {
        if (op == ""){ 
            // LAndExp := EqExp
            return subExp->DumpIR();
        }
        
        // LAndExp := LAndExp LANDOP EqExp
        assert(op == "&&");
        std::string landexp = lAndExp->DumpIR();
        std::string subexp = subExp->DumpIR();
        std::cout << "\t%" << varNum++ << " = ne " << landexp << ", 0\n";
        std::cout << "\t%" << varNum++ << " = ne " << subexp << ", 0\n";
        std::cout << "\t%" << varNum << " = and %" << (varNum - 2) << ", %" 
                    << (varNum - 1) << "\n";
        return "%" + std::to_string(varNum++);
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
    void Dump() const override
    {
        if (op == ""){ 
            // LOrExp := LAndExp
            subExp->Dump();
        }
        else{
            // LOrExp := LOrExp LOROP LAndExp
            lOrExp->Dump();
            std::cout << op;
            subExp->Dump();
        }
    }

    std::string DumpIR() const override{
        if (op == ""){ 
            // LOrExp := LAndExp
            return subExp->DumpIR();
        }
        
        // LOrExp := LOrExp LOROP LAndExp
        assert(op == "||");
        std::string lorexp = lOrExp->DumpIR();
        std::string subexp = subExp->DumpIR();
        std::cout << "\t%" << varNum++ << " = eq " << lorexp << ", 0\n";
        std::cout << "\t%" << varNum++ << " = eq " << subexp << ", 0\n";
        std::cout << "\t%" << varNum << " = or %" << (varNum - 2) << ", %" 
                    << (varNum - 1) << "\n";
        return "%" + std::to_string(varNum++);
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

inline static SymTabEntry symTabs_lookup(std::string lval){
    for (auto it = symTabs.rbegin(); it != symTabs.rend(); it++){
        if (it->count(lval))
            return (*it)[lval];    
    }
    assert(false);
    return 0;
} 
