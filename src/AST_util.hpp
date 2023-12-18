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

inline SymTabEntry symTabs_lookup(std::string lval);
inline std::string getNewSymName();
