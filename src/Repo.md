# 编译原理课程实践报告：Compiler-

信息科学技术学院 2100013018 张龄心

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 将`SysY`语言程序编译为`Koopa IR`
2. 将`Koopa IR`编译为`RISC-V`指令
3. 实现了`RISC-V`寄存器分配策略, 以达到优化性能的目的

### 1.2 主要特点

我开发的编译器的主要特点是实现简单、正确性高，并且进行了部分性能优化。编译过程完全利用嵌套的 AST 数据结构递归实现，只需一次遍历，代码简洁；通过了性能测试的所有测试用例以及 Koopa IR 和 RISC-V 的几乎全部测试用例，正确性较高；使用分级的寄存器分配策略，提高了性能。

## 二、编译器设计

### 2.1 主要模块组成

编译器由3个主要模块组成.
1. 词法分析和语法分析: `sysy.l` 和 `sysy.y`
2. 前端(从 SysY 到 Koopa IR): `AST.h` 和 `AST_util.h`
3. 后端(从 Koopa IR 到 RISC-V ): `riscv.h` 和 `riscv_util.h`

### 2.2 主要数据结构

#### 2.2.1 抽象语法树
本编译器最核心的数据结构是抽象语法树AST. 在实际代码编写过程中，我沿用了文档中的数据结构`BaseAST`作为基类, 所有其他的AST子结构都是`BaseAST`的衍生类, 例如`class CompUnitAST`, `class FuncDefAST`等等.

如果将一个SysY程序视作一棵树，那么一个`CompUnitAST`的实例就是这棵树的根. 因此, 在生成Koopa IR时, 只需调用`CompUnitAST`中的`Dump()`函数, 该函数会递归地调用相关子类中的`Dump()`, `DumpGlobal()`等函数, 从而完成Koopa IR的生成, 例如`Dump()`用于生成子结构的IR, `DumpGlobal()`主要用于生成全局变量, `CalValue()`用于计算常量的值, 等等.


```c
// Base class for all ASTs
class BaseAST {
    public:
    bool isEmptyInitArray = 0; // For InitValAST. If the Initialization is like "int[10] a = {}".
    int32_t arrayDimension = -1; // For FuncFParam. Set to the array dimension if the parameter is an array

    virtual ~BaseAST() = default;

    virtual std::string Dump() const{ assert(0); return 0; } // for local KoopaIR generation
    virtual std::string DumpArray() const{ assert(0); return 0; } // called in Dump() for lv9
    virtual std::string DumpGlobal() const{ assert(0); return ""; } // for global variable generation
    virtual std::string DumpGlobalArray() const{ assert(0); return ""; } // called in DumpGlobal() for lv9
    virtual int32_t CalValue() const{ assert(0); return -1; }
    virtual std::string GetIdent() const{ assert(0); return ""; }
    virtual std::string GetType() const{ assert(0); return ""; }
    virtual std::string TypeOfAST() const{ return ""; }
    virtual std::vector<int32_t> GetArrayInitVals(std::vector<int32_t>) const{ 
        assert(0); return std::vector<int32_t>(); }
    virtual bool GetIsArray() const { assert(0); return 0;}
};
```

#### 2.2.2 RISC-V指令信息
在RISC-V的生成中, 定义了存储某条指令相关信息的类`RegInfo`, 其中, `regnum`是存放该指令运算结果的寄存器编号, `offset`是该指令在栈上的偏移量, 若无相关信息则设为-1.
```c
class RegInfo {
    public:
        int32_t regnum = -1;
        int32_t offset = -1; 

        RegInfo(){};
        RegInfo(int32_t rn, int32_t ofs): regnum(rn), offset(ofs) {}

        inline void printRegInfo(){
            std::cout << "// regnum = " << regnum << ", offset = " << offset << "\n";
            return;
        }
};
```
另外用一个`unordered_map`保存了指令到其信息的对应关系. 
```c
std::unordered_map<koopa_raw_value_t, RegInfo> RegInfoMap;
```

#### 2.2.3 符号表
在IR生成的过程中, 维护了一个符号表`symTabs`, 支持作用域嵌套, 选用栈式结构来存储多层符号表询.

多层嵌套符号表的定义如下: 
```c
static std::vector<SymTabType> symTabs;
```
其中, SymTabType是符号表的类型, 定义如下: 
```c
typedef std::variant<int, std::string> SymTabEntry;
typedef std::unordered_map<std::string, SymTabEntry> SymTabType; // Map ident to integer value
```
`SymTabEntry`是符号表条目.
更多细节见2.3.1节.

#### 2.2.4 其他存储信息的结构

为了存储一些IR生成中可能用到的其他信息, 设置了一些其他的存储信息的数据结构. 大多数采用的是`unordered_map`, 以便查询.
```c
static std::unordered_map<std::string, bool> isParam; // Is this symbol a parameter name?
static std::unordered_map<std::string, int32_t> symNameCount; // Map an ident to its count
static std::vector<int32_t> whileTab; // Recording current while layers
static std::unordered_map<std::string, FuncInfo> FuncInfoList; // Map a function name to its info class
static std::unordered_map<std::string, int32_t> ArrayDimension; // The dimension of a given array.
static std::unordered_map<std::string, bool> isArrayGlobal; // To exclude array parameter
static std::vector<int32_t> ArrayInitVals;
```

其中`FuncInfo`的定义如下:
```c
typedef struct {
    std::string ident;
    std::string name;
    std::string type;       
}ParamInfo;

typedef struct{
    std::string retType;
    std::vector<ParamInfo> paramInfoList;
} FuncInfo;
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑
为了支持作用域嵌套, 选用栈式结构来存储多层符号表, 具体实现为vector, 以便查询.

多层嵌套符号表的定义如下: 
```c
static std::vector<SymTabType> symTabs;
```
其中, SymTabType是符号表的类型, 定义如下: 
```c
typedef std::variant<int, std::string> SymTabEntry;
typedef std::unordered_map<std::string, SymTabEntry> SymTabType; // Map ident to integer value
```
`SymTabEntry`是符号表条目. 符号表会将一个变量名映射到一个int(对于常量)或一个string(对于变量).

初始化时, 在`SymTabs`中插入第一个元素, 作为全局符号表. 
每次进入一个新的子作用域, 就创建一个新的符号表(类型为`SymTabType`), 然后插入`symTabs`的尾部; 每次退出当前子作用域, 就从`symTabs`尾部弹出一个符号表. 

查询一个符号时, 先在`symTabs`尾部的符号表查询, 如果找不到, 再去上一个符号表中查询, 以此类推.

#### 2.3.2 寄存器分配策略
采取了一种简单的分配策略, 参考了[这个实现](https://github.com/GeorgeMLP/sysy-compiler/tree/master).

大致思路: 定义了一个优先级enum, 为每个寄存器维护一个当前优先级状态, 包括`high` / `mid` / `low`三类:
```c
enum Priority {low = 0, mid = 1, high = 2};
```

`high`表示该寄存器中的内容很重要, 不可修改; `mid`表示该寄存器中的内容仍然可能用到, 但是也可以保存到栈上以腾出空间; `low`表示该寄存器中的内容不再需要, 寄存器可以随意使用. 初始时所有寄存器的优先级均为`low`.

需要一个寄存器时, 分配算法扫描所有寄存器, 寻找优先级为`low`的寄存器, 如有, 选择它. 如果没有优先级为`low`的寄存器, 再次扫描寻找优先级为`mid`的寄存器, 如有, 将它原先的值保存到栈中, 再选择它. 

算法的设计确保不会出现所有优先级均为`high`的情况.

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成
基本上参考文档的写法即可, 主要是搭建了代码的基本框架. 
在写之前看了若干网上的实现, 因此决定设置`AST_util.h`和`riscv_util.h`, 以使代码更简洁.

#### Lv3. 表达式
这部分针对各种形式的表达式, 在`sysy.y`中添加了对应部分(基本上对着语法规范写即可):

```c
// sysy.y
%type <ast_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
```

在`AST.h`中添加了`ExpAST` `PrimaryExpAST`等子类别, 用于对应的各级表达式; 并在`riscv.h`中添加了相应基本指令的生成.

#### Lv4. 常量和变量
本章添加`class DeclAST` 用于表示声明.

针对常量, `class ConstDeclAST`,`class ConstDefAST`和`class ConstInitValAST` 用于表示常量声明,定义和初始值; 同时, 在相关子类中设置`CalValue()`函数来计算常量的值. 以`LOrExpAST`中为例: 
```c
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
```

针对变量, `class VarDeclAST`,`class VarDefAST`和`class InitValAST` 用于表示变量声明,定义和初始值. 它们递归调用生成相应中间表示. 以第一个为例: 
```c
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
```

对于目标代码生成, 维护了一个`RegInfoMap`, 将一条指令映射到其存储位置(寄存器编号或栈上的偏移量):
```c
std::unordered_map<koopa_raw_value_t, RegInfo> RegInfoMap;
```

同时, 需要一个基本的符号表, 将变量名映射到其值. 当然, 这一符号表很快就在lv5中被修改.

#### Lv5. 语句块和作用域
基本上和lv4一起实现. 这里主要的改动在于前述的符号表, 需要设置栈式结构, 前面已经阐述过, 不再赘述.

#### Lv6. if语句
简单的if-else没什么好说的, 只需要注意对if进行计数, 以确保跳转的label不重复. 另外在有些cases中包含`end_0`之类的阴间变量名, 我的解决方法是把label名改成了形如`_end_0`的形式. 
关于二义性, 参考了书上悬空if-else问题的相关讨论, 使用open statement和closed statement的写法.

在`sysy.y`中设置`ComplexStmt`类作为大类, 包含`OpenStmt`和`ClosedStmt`. 最简单的子表达式(子语句)是`Stmt`. 同时, 在`AST.h`中设置了`ComplexStmtAST`类和`StmtAST`类.
```c
ComplexStmt
    : OpenStmt {
        $$ = ($1);
    }
    | ClosedStmt {
        $$ = ($1);
    }
    ;

ClosedStmt
  : Stmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_simple;
    stmt->subExp = BaseASTPtr($1);
    $$ = stmt;
  }
  | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_ifelse;
    stmt->subExp = BaseASTPtr($3);
    stmt->subStmt = BaseASTPtr($5);
    stmt->elseStmt = BaseASTPtr($7);
    $$ = stmt;
  };

OpenStmt
  : IF '(' Exp ')' ComplexStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_openif;
    stmt->subExp = BaseASTPtr($3);
    stmt->subStmt = BaseASTPtr($5);
    $$ = stmt;
  }
  | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_ifelse;
    stmt->subExp = BaseASTPtr($3);
    stmt->subStmt = BaseASTPtr($5);
    stmt->elseStmt = BaseASTPtr($7);
    $$ = stmt;
  };

Stmt {...}
```

为了实现短路求值, 修改了`LorExp`和`LAndExp`的相关`Koopa IR`生成, 这里直接将OR和AND运算视为一个if-else来处理了. 

以OR运算为例, 大致思路是: if(第一个判断为真), then(jump到第二个判断之后).
```c
class LOrExpAST : public BaseAST{
    public:
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
}
```

#### Lv7. while语句
包括`while` `break` `continue`. 相关实现类似`if-else`, 都是加入一些label并且设置相应的跳转.

`while`实现在`ComplexStmt`中, `break` `continue`实现在`Stmt`中. 以`while`为例: 
```c
class ComplexStmtAST : public BaseAST{
    public:    
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
            /*. . . */
        }
}
```

#### Lv8. 函数和全局变量
需要处理参数保存和函数调用两方面. 
IR生成较为简单, 只需要在`UnaryExpAST`中实现调用过程即可:
```c
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
```

主要的困难在于目标代码生成. 需要在调用前将参数移到相应寄存器中, 注意参数数量大于8时要将多出来的参数放在栈上.
```c
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
```
在函数开头要计算栈的偏移量, 视情况减少栈指针和保存`ra`寄存器. 函数调用前要保存所有优先级非low的寄存器,  并舍弃所有其他寄存器的值.

对于全局变量, 在IR生成时, 专门调用`DumpGlobal()`函数即可, 同时实现可能需要的初始化. 

在目标代码生成时, 则需要额外的一些处理:

```c
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
```

#### Lv9. 数组
多维数组涉及大量递归. 这里使用了一系列成员函数和工具函数来实现:

`DumpArray()`和`DumpGlobalArray()`专门用于打印数组:
```c
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
```

`GetArrayInitVals`存储对应的初始值, 以便后续的初始化: 
```c
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
```

`ArrayInit()`对数列进行初始化: 
```c
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
```

`printArrayInit()`打印初始化的代码:
```c
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
```

另外还用到计算Type和Size的工具函数:
```c
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
```


在目标代码生成部分, 根据具体类型, 选择`get_elem_ptr`和`get_ptr`:
```c
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
}```

### 3.2 工具软件介绍（若未使用特殊软件或库，则本部分可略过）
1. `flex/bison`：用以完成词法分析和语法分析，生成语法分析树。
2. `libkoopa`：将文本形式的IR转化为`Koopa IR`.
   `Git`和`Docker`: Git用于控制版本, Docker用于配环境.


## 四、实习总结

### 4.1 收获和体会

第一次实现一个这么大型的程序, 的确非常有成就感! 不得不说, 自己完成这样的任务虽然很辛苦, 但是的确收获很大. 

如果说细节一点的东西, 可能是进一步熟悉了Git的使用 / Docker的使用 / vscode的快捷键, 复习了C++和stl库, 等等.

### 4.2 学习过程中的难点，以及对实习过程和内容的建议
1. 可以考虑完善一些文档的细节, 主要是添加更多更详细的例子. 
2. 可以考虑设置若干阶段性的ddl, 而不是整个学期一个ddl.
   也许可以分拆成3-4个小的ddl, 例如lv0-2一个Lab, lv3-lv5一个Lab, 等等. 这样一方面可以更好地和课程进度贴合, 另一方面也可以减少集中赶ddl乃至赶不上ddl的情况.

   以我的体感而言, 编译Lab的辛苦程度远大于ICS, 甚至远大于jx老师操统实验班的PintOS. 这一方面是因为任务量的确非常大, 但是另一方面也是因为只有一个期末ddl, 所以前半学期很容易拖进度(x), 导致后半学期集中肝起来体验很糟糕. 

   参考: 操统实验班也是整个学期实现一个OS, 但是拆成了Lab0-Lab4, 每个Lab都是在前一个Lab的基础上完成的, 最终呈现的是一个完整的OS.

### 4.3 对老师讲解内容与方式的建议

目前感觉在日常的教学进度中, 编译Lab和课程仍然是关联度不大的, 直到期末的时候才有一种Lab和课程内容融会贯通了的感觉. 希望这一关联能够增强.
