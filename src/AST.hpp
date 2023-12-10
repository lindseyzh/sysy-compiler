#pragma once
#include<memory>
#include<string>
#include<iostream>
#include<cassert>

static int varNum = 0;


// Base class for all ASTs
class BaseAST {
    public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
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
        func_def->DumpIR();
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

class BlockAST : public BaseAST {
    public: 
        std::unique_ptr<BaseAST> stmt;
    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }" << std::endl;
    }
    std::string DumpIR() const override {
        stmt->DumpIR();
        return "";
    }
};

class StmtAST : public BaseAST {
    public: 
        std::unique_ptr<BaseAST> subExp;
    void Dump() const override {
        std::cout << "StmtAST { ";
        subExp->Dump();
        std::cout << " }";
    }
    std::string DumpIR() const override {
        std::string subexp = subExp->DumpIR();
        std::cout << "\tret ";
        std::cout << subexp << "\n";
        return "";
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
    // virtual int dumpExp() const override{
    //     return unaryexp->dumpExp();
    // }
};

class PrimaryExpAST : public BaseAST
{
public:
    enum {def_bracketexp, def_numberexp} def;
    std::unique_ptr<BaseAST> subExp;
    // std::string lval;
    // std::vector<std::unique_ptr<BaseAST>> exp_list;
    int number;
    void Dump() const override
    {
        if (def == def_bracketexp) {
            subExp->Dump();
        }
        else if (def == def_numberexp){
            std::cout << number;
        }
        // else if (type == PrimaryExpType::lval)std::cout << lval;
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

    std::string DumpIR() const override{
        std::string result_var = "";
        if (def == def_bracketexp){
            return subExp->DumpIR();
        }
        else if (def == def_numberexp){
            return std::to_string(number);
        }
        /*
        else if (type == PrimaryExpType::lval)
        {
            std::variant<int, std::string> value = look_up_symbol_tables(lval);
            if (value.index() == 0)
                result_var = std::to_string(std::get<int>(value));
            else if (is_list[std::get<std::string>(value)])
            {
                result_var = "%" + std::to_string(symbol_num++);
                std::cout << '\t' << result_var << " = getelemptr " <<
                    std::get<std::string>(value) << ", 0" << std::endl;
            }
            else
            {
                result_var = "%" + std::to_string(symbol_num++);
                std::cout << '\t' << result_var << " = load " <<
                    std::get<std::string>(value) << std::endl;
            }
        }
        else if (type == PrimaryExpType::list)
        {
            std::variant<int, std::string> value = look_up_symbol_tables(lval);
            assert(value.index() == 1);
            std::string name, prev = std::get<std::string>(value);
            int dim = list_dim[prev];
            bool list = is_list[prev], func_param = is_func_param[prev];
            for (auto&& exp : exp_list)
            {
                result_var = exp->dumpIR();
                name = "%" + std::to_string(symbol_num++);
                if (is_func_param[prev])
                {
                    std::cout << '\t' << name << " = load " << prev <<
                        std::endl;
                    std::string tmp = "%" + std::to_string(symbol_num++);
                    std::cout << '\t' << tmp << " = getptr " << name <<
                        ", " << result_var << std::endl;
                    name = tmp;
                }
                else
                    std::cout << '\t' << name << " = getelemptr " << prev <<
                        ", " << result_var << std::endl;
                prev = name;
            }
            if (exp_list.size() == dim)
            {
                result_var = "%" + std::to_string(symbol_num++);
                std::cout << '\t' << result_var << " = load " << prev <<
                    std::endl;
            }
            else if (list || func_param)
            {
                result_var = "%" + std::to_string(symbol_num++);
                std::cout << '\t' << result_var << " = getelemptr " << prev <<
                    ", 0" << std::endl;
            }
            else result_var = name;
        }
        else assert(false);
        */
        return "";
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
        // TODO: Handle And operation (A && B is considered (A!=0) && (B!=0) here).
        std::cout << "\t%" << varNum++ << " = ne " << landexp << ", 0\n";
        std::cout << "\t%" << varNum++ << " = ne " << subexp << ", 0\n";
        std::cout << "\t%" << varNum << " = and %" << (varNum - 2) << ", %" 
                    << (varNum - 1) << "\n";
        return "%" + std::to_string(varNum++);
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
    std::string DumpIR() const override
    {
        if (op == ""){ 
            // LOrExp := LAndExp
            return subExp->DumpIR();
        }
        
        // LOrExp := LOrExp LOROP LAndExp
        assert(op == "||");
        std::string lorexp = lOrExp->DumpIR();
        std::string subexp = subExp->DumpIR();
        // TODO: Handle And operation (A && B is considered (A!=0) && (B!=0) here).
        std::cout << "\t%" << varNum++ << " = eq " << lorexp << ", 0\n";
        std::cout << "\t%" << varNum++ << " = eq " << subexp << ", 0\n";
        std::cout << "\t%" << varNum << " = or %" << (varNum - 2) << ", %" 
                    << (varNum - 1) << "\n";
        return "%" + std::to_string(varNum++);
    }
};
