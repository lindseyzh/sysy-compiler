#pragma once
#include<memory>
#include<string>
#include<iostream>
#include<cassert>
#include<vector>
#include<map>
#include<set>
#include<unordered_map>
#include<unordered_set>
#include<variant>

class BaseAST;
class InitValAST;
typedef std::unique_ptr<BaseAST> BaseASTPtr;
typedef std::variant<int, std::string> SymTabEntry;
typedef std::unordered_map<std::string, SymTabEntry> SymTabType; // Map ident to integer value / @ident_Num
typedef std::vector<BaseASTPtr> MulVecType;

typedef struct {
    std::string ident;
    std::string name;
    std::string type;       
}ParamInfo;

typedef struct{
    std::string retType;
    std::vector<ParamInfo> paramInfoList;
} FuncInfo;