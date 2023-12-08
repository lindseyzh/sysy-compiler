#pragma once
#include<memory>
#include<string>
#include<iostream>

// Base class for all ASTs
class BaseAST {
    public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
    virtual void DumpIR() const = 0;
};

class CompUnitAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    void DumpIR() const override {
        func_def->DumpIR();
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

    void DumpIR() const override {
        std::cout << "fun @" << ident << "(): ";
        std::cout << "i32";
        std::cout << " {" << "\n";
        std::cout << "%entry:" << "\n";
        block->DumpIR();
        std::cout << "}" << std::endl;;
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
    void DumpIR() const override {
        stmt->DumpIR();
        return;
    }
};

class StmtAST : public BaseAST {
    public: 
        std::unique_ptr<int> number;
    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << *number << " }";
    }
    void DumpIR() const override {
        std::cout << "ret " << *number << "\n";
    }
};
