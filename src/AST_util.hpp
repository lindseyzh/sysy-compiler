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
typedef std::unordered_map<std::string, SymTabEntry> SymTabType; // Map ident to integer value / @ident_Num
typedef std::vector<std::unique_ptr<BaseAST>> MulVecType;

typedef struct {
    std::string ident;
    std::string name;
    std::string type;       
}ParamInfo;

typedef struct{
    std::string retType;
    std::vector<ParamInfo> paramInfoList;
} FuncInfo;

inline SymTabEntry symTabs_lookup(std::string lval);
inline std::string getNewSymName();
inline void libFuncDecl();
