#pragma once
#include<memory>
#include<string>
#include<iostream>

// Base class for all ASTs
class BaseAST {
    public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
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
        std::cout << " }";
  }
};

class BlockAST : public BaseAST {
    public: 
        std::unique_ptr<BaseAST> stmt;
    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }
};

class StmtAST : public BaseAST {
    public: 
        std::unique_ptr<int> number;
    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << *number << " }";
    }
};
