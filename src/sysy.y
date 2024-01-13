// 当前状态: lv9 case 6 yyparse出错. 需要检查词法/语法分析相关. 
// 另外, 前面的cases中, 初始化的值很奇怪, 有些该初始化为0的值被初始化成其他值了, 尚不知道原因
// 可能是sysy.y到AST的时候, 某些结构的AST出错? 但是并没有报段错误, 只是分析失败而已.


%code requires {
  #include <memory>
  #include <string>
  #include "AST.hpp"
}

%{

#include <iostream>
#include<cstring>
#include <memory>
#include <string>
#include<vector>
#include<map>
#include "AST.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(BaseASTPtr &ast, const char *s);

using namespace std;

%}

%parse-param { BaseASTPtr &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  MulVecType *mul_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST VOID IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT UNARYOP MULOP ADDOP RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block BlockItem Stmt ComplexStmt OpenStmt ClosedStmt 
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal
%type <ast_val> Exp ConstExp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <mul_val> BlockItems ConstDefs VarDefs FuncFParams FuncRParams
%type <mul_val> InitVals ConstInitVals ExpArray ConstExpArray
%type <ast_val> CompUnitList FuncFParam
%type <int_val> Number
%type <str_val> Type LVal

%%

CompUnit
  : CompUnitList {
    auto comp_unit = BaseASTPtr($1);
    ast = std::move(comp_unit);
  }
  ;

CompUnitList
  : FuncDef {
    auto comp_unit = new CompUnitAST();
    auto func_def = BaseASTPtr($1);
    comp_unit->funcDefs.push_back(std::move(func_def));
    $$ = comp_unit;
  }
  | Decl {
    auto comp_unit = new CompUnitAST();
    auto decl = BaseASTPtr($1);
    comp_unit->decls.push_back(std::move(decl));
    $$ = comp_unit;
  }
  | CompUnitList FuncDef {
    auto comp_unit = (CompUnitAST*)($1);
    auto func_def = BaseASTPtr($2);
    comp_unit->funcDefs.push_back(std::move(func_def));
    $$ = comp_unit;
  }
  | CompUnitList Decl {
    auto comp_unit = (CompUnitAST*)($1);
    auto decl = BaseASTPtr($2);
    comp_unit->decls.push_back(std::move(decl));
    $$ = comp_unit;
  }
  ;

// Note: $n = the nth parameter
FuncDef
  : Type IDENT '(' ')' Block {
    auto func_def = new FuncDefAST();
    func_def->funcType = *unique_ptr<string>($1);
    func_def->ident = *unique_ptr<string>($2);
    func_def->block = BaseASTPtr($5);
    $$ = func_def;
  }
  | Type IDENT '(' FuncFParams ')' Block {
    auto func_def = new FuncDefAST();
    func_def->funcType = *unique_ptr<string>($1);
    func_def->ident = *unique_ptr<string>($2);
    MulVecType *vec = ($4);
    for (auto it = vec->begin(); it != vec->end(); it++)
        func_def->funcFParams.push_back(std::move(*it));
    func_def->block = BaseASTPtr($6);
    ((BlockAST*)(func_def->block).get())->func = func_def->ident;
    $$ = func_def;
  }
  ;

FuncFParams
  : FuncFParam {
    MulVecType *vec = new MulVecType;
    vec->push_back(BaseASTPtr($1));
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    MulVecType *vec = ($1);
    vec->push_back(BaseASTPtr($3));
    $$ = vec;
  }
  ;

FuncFParam
  : Type IDENT {
    auto func_f_param = new FuncFParamAST();
    func_f_param->def = FuncFParamAST::def_common;
    func_f_param->bType = *unique_ptr<string>($1);
    func_f_param->ident = *unique_ptr<string>($2);
    $$ = func_f_param;
  }
  | Type IDENT '[' ']' {
    auto func_f_param = new FuncFParamAST();
    func_f_param->def = FuncFParamAST::def_array;
    func_f_param->bType = *unique_ptr<string>($1);
    func_f_param->ident = *unique_ptr<string>($2);
    func_f_param->arrayDimension = 1;
    $$ = func_f_param;
  }
  | Type IDENT '[' ']' ConstExpArray {
    auto func_f_param = new FuncFParamAST();
    func_f_param->def = FuncFParamAST::def_array;
    func_f_param->bType = *unique_ptr<string>($1);
    func_f_param->ident = *unique_ptr<string>($2);
    MulVecType *vec = ($5);
    for (auto it = vec->begin(); it != vec->end(); it++)
        func_f_param->constExpArray.push_back(std::move(*it));
    func_f_param->arrayDimension = vec->size() + 1;
    $$ = func_f_param;
  }
  ;

FuncRParams
  : Exp {
    MulVecType *vec = new vector<BaseASTPtr>;
    vec->push_back(BaseASTPtr($1));
    $$ = vec;
  }
  | FuncRParams ',' Exp {
    MulVecType *vec = ($1);
    vec->push_back(BaseASTPtr($3));
    $$ = vec;
  }
  ;

Type
  : INT {
    $$ = new string("int");
  }
  | VOID {
    $$ = new string("void");
  }
  ;

Decl
  : ConstDecl {
    auto decl = new DeclAST();
    decl->def = DeclAST::def_const;
    decl->decl = BaseASTPtr($1);
    $$ = decl;
  }
  | VarDecl {
    auto decl = new DeclAST();
    decl->def = DeclAST::def_var;
    decl->decl = BaseASTPtr($1);
    $$ = decl;
  }
  ;

ConstDecl
  : CONST Type ConstDefs ';' {
    auto const_decl = new ConstDeclAST();
    const_decl->bType = *unique_ptr<string>($2);
    MulVecType *vec = ($3);
    for (auto it = vec->begin(); it != vec->end(); it++)
      const_decl->constDefs.push_back(std::move(*it));
    $$ = const_decl;
  }
  ;

ConstDefs
  : ConstDef {
    auto const_defs = new MulVecType;
    const_defs->push_back(BaseASTPtr($1));
    $$ = const_defs;
  }
  | ConstDefs ',' ConstDef {
    MulVecType *const_defs = ($1);
    const_defs->push_back(BaseASTPtr($3));
    $$ = const_defs;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
      auto const_def = new ConstDefAST();
      const_def->ident = *unique_ptr<string>($1);
      const_def->constInitVal = BaseASTPtr($3);
      $$ = const_def;
  }
  | IDENT ConstExpArray '=' ConstInitVal {
    auto const_def = new ConstDefAST();
    const_def->ident = *unique_ptr<string>($1);
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
        const_def->constExpArray.push_back(std::move(*it));
    const_def->constInitVal = BaseASTPtr($4);
    const_def->isArray = 1;
    $$ = const_def;
  }
  ;

ConstInitVal
  : ConstExp {
      auto const_init_val = new ConstInitValAST();
      const_init_val->subExp = BaseASTPtr($1);
      $$ = const_init_val;
  }
  | '{' '}' {
    auto const_init_val = new ConstInitValAST();
    const_init_val->isArray = 1;
    const_init_val->isEmptyInitArray = 1;
    $$ = const_init_val;
  }
  | '{' ConstInitVals '}' {
    auto const_init_val = new ConstInitValAST();
    const_init_val->isArray = 1;
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
        const_init_val->constInitVals.push_back(move(*it));
    $$ = const_init_val;
  }
  ;

ConstInitVals
  : ConstInitVal {
    MulVecType *v = new MulVecType;
    v->push_back(BaseASTPtr($1));
    $$ = v;
  }
  | ConstInitVals ',' ConstInitVal {
    MulVecType *v = ($1);
    v->push_back(BaseASTPtr($3));
    $$ = v;
  }
  ;

VarDecl
  : Type VarDefs ';' {
    auto var_decl = new VarDeclAST();
    var_decl->bType = *unique_ptr<string>($1);
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
        var_decl->varDefs.push_back(std::move(*it));
    $$ = var_decl;
  }
  ;

VarDefs
  : VarDef {
    auto var_defs = new MulVecType;
    var_defs->push_back(BaseASTPtr($1));
    $$ = var_defs;
  }
  | VarDefs ',' VarDef {
    MulVecType *var_defs = ($1);
    var_defs->push_back(BaseASTPtr($3));
    $$ = var_defs;
  }
  ;

VarDef
  : IDENT {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    $$ = var_def;
  }
  | IDENT '=' InitVal {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    var_def->initVal = BaseASTPtr($3);
    $$ = var_def;
  }
  | IDENT ConstExpArray {
    auto var_def = new VarDefAST();
    var_def->ident = *unique_ptr<string>($1);
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
        var_def->constExpArray.push_back(std::move(*it));
    $$ = var_def;
  }
  | IDENT ConstExpArray '=' InitVal {
    auto var_def = new VarDefAST();
    var_def->isInitialized = 1;
    var_def->ident = *unique_ptr<string>($1);
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
        var_def->constExpArray.push_back(std::move(*it));
    var_def->initVal = BaseASTPtr($4);
    $$ = var_def;
  }
  ;

// TODO: change all vector sysy definitions about '{' '}'
InitVal
  : Exp {
    auto init_val = new InitValAST();
    init_val->subExp = BaseASTPtr($1);
    $$ = init_val;
  }
  | '{' '}' {
    auto init_val = new InitValAST();
    init_val->isArray = 1;
    init_val->isEmptyInitArray = 1;
    $$ = init_val;
  }
  | '{' InitVals '}' {
    auto init_val = new InitValAST();
    MulVecType *v_ptr = ($2);
    for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
        init_val->initVals.push_back(move(*it));
    init_val->isArray = 1;
    $$ = init_val;
  }

  ;

InitVals
  : InitVal {
    MulVecType *v = new MulVecType;
    v->push_back(BaseASTPtr($1));
    $$ = v;
  }
  | InitVals ',' InitVal {
    MulVecType *v = ($1);
    v->push_back(BaseASTPtr($3));
    $$ = v;
  }
  ;

Block
  : '{' BlockItems '}' {
    auto block = new BlockAST();
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      block->blockItems.push_back(std::move(*it));
    $$ = block;
  }
  | '{' '}' {
    $$ = new BlockAST();
    // Do nothing
  }
  ;

BlockItems
  : BlockItem {
    auto block_items = new MulVecType;
    block_items->push_back(BaseASTPtr($1));
    $$ = block_items;
  }
  | BlockItems BlockItem {
    MulVecType *block_items = ($1);
    block_items->push_back(BaseASTPtr($2));
    $$ = block_items;
  }
  ;

BlockItem
  : Decl {
    // std::cout << "// BlockItem-Decl\n"; // for debug
    auto block_item = new BlockItemAST();
    block_item->def = BlockItemAST::def_decl;
    block_item->blockItem = BaseASTPtr($1);
    $$ = block_item;
  }
  | ComplexStmt {
    // std::cout << "// BlockItem-stmt\n"; // for debug
    auto block_item = new BlockItemAST();
    block_item->def = BlockItemAST::def_stmt;
    block_item->blockItem = BaseASTPtr($1);
    $$ = block_item;  }
  ;

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
  }
  | WHILE '(' Exp ')' ClosedStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_while;
    stmt->subExp = BaseASTPtr($3);
    stmt->subStmt = BaseASTPtr($5);
    $$ = stmt;
  }
  ;

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
  }    
  | WHILE '(' Exp ')' OpenStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_while;
    stmt->subExp = BaseASTPtr($3);
    stmt->subStmt = BaseASTPtr($5);
    $$ = stmt;
  }  
  ;

Stmt
  : RETURN Exp ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_ret;
    stmt->subExp = BaseASTPtr($2);
    $$ = stmt;
  }
  | RETURN ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_ret;
    $$ = stmt;
  }
  | LVal '=' Exp ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_lval;
    stmt->lVal = *unique_ptr<string>($1);
    stmt->subExp = BaseASTPtr($3);
    $$ = stmt;
  }
  | Exp ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_exp;
    stmt->subExp = BaseASTPtr($1);
    $$ = stmt;
  }
  | ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_exp;
    $$ = stmt;
  }
  | Block {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_block;
    stmt->subExp = BaseASTPtr($1);
    $$ = stmt;
  }
  | BREAK ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_break;
    $$ = stmt;
  }
  | CONTINUE ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_continue;
    $$ = stmt;
  } 
  | IDENT ExpArray '=' Exp ';' {
    auto stmt = new StmtAST();
    stmt->lVal = *unique_ptr<string>($1);
    stmt->def = StmtAST::def_array;
    stmt->subExp = BaseASTPtr($4);
    MulVecType *v_ptr = ($2);
    for (auto it = v_ptr->begin(); it != v_ptr->end(); it++)
        stmt->expArray.push_back(std::move(*it));
    $$ = stmt;
  }
  ;

LVal
  : IDENT {
    string *lval = new string(*unique_ptr<string>($1));
    $$ = lval;
  }
  ;

ConstExp 
  : Exp {
    auto const_exp = new ConstExpAST();
    const_exp->subExp = BaseASTPtr($1);
    $$ = const_exp;
  }
  ;

ConstExpArray
  : '[' ConstExp ']' {
    MulVecType *v = new MulVecType;
    v->push_back(BaseASTPtr($2));
    $$ = v;
  }
  | ConstExpArray '[' ConstExp ']' {
    MulVecType *v = ($1);
    v->push_back(BaseASTPtr($3));
    $$ = v;
  }
  ;

Exp
  : LOrExp {
      auto exp = new ExpAST();
      exp->subExp = BaseASTPtr($1);
      $$ = exp;
  }
  ;

ExpArray
  : '[' Exp ']' {
    MulVecType *v = new MulVecType;
    v->push_back(BaseASTPtr($2));
    $$ = v;
  }
  | ExpArray '[' Exp ']' {
    MulVecType *v = ($1);
    v->push_back(BaseASTPtr($3));
    $$ = v;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->def = PrimaryExpAST::def_bracketexp;
    primary_exp->subExp = BaseASTPtr($2);
    $$ = primary_exp;
  }
  | LVal {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->def = PrimaryExpAST::def_lval;
    primary_exp->lVal = *unique_ptr<string>($1);
    $$ = primary_exp;
  }
  | Number {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->def = PrimaryExpAST::def_number;
    primary_exp->number = ($1);
    $$ = primary_exp;
  }
  | IDENT ExpArray {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->def = PrimaryExpAST::def_array;
    primary_exp->arrayIdent = *unique_ptr<string>($1);
    MulVecType *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
        primary_exp->expArray.push_back(std::move(*it));
    $$ = primary_exp;
  }
  ;

Number
  : INT_CONST {
    $$ = ($1);
  }
  ;

UnaryExp
  : PrimaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_primaryexp;
    unary_exp->subExp = BaseASTPtr($1);
    $$ = unary_exp;
  }
  | ADDOP UnaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_unaryexp;
    unary_exp->op = *unique_ptr<string>($1);
    unary_exp->subExp = BaseASTPtr($2);
    $$ = unary_exp;
  }
  | UNARYOP UnaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_unaryexp;
    unary_exp->op = *unique_ptr<string>($1);
    unary_exp->subExp = BaseASTPtr($2);
    $$ = unary_exp;
  }
  | IDENT '(' ')' {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_func;
    unary_exp->ident = *unique_ptr<string>($1);
    $$ = unary_exp;
  }
  | IDENT '(' FuncRParams ')' {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_func;
    unary_exp->ident = *unique_ptr<string>($1);
    vector<BaseASTPtr> *vec = ($3);
    for (auto it = vec->begin(); it != vec->end(); it++)
        unary_exp->funcRParams.push_back(std::move(*it));
    $$ = unary_exp;
  }
  ;

MulExp
  : UnaryExp {
      auto exp = new MulExpAST();
      exp->subExp = BaseASTPtr($1);
      $$ = exp;
  }
  | MulExp MULOP UnaryExp {
      auto exp = new MulExpAST();
      exp->mulExp = BaseASTPtr($1);
      exp->op = *unique_ptr<string>($2);
      exp->subExp = BaseASTPtr($3);
      $$ = exp;
  }
  ;

AddExp
  : MulExp {
      auto exp = new AddExpAST();
      exp->subExp = BaseASTPtr($1);
      $$ = exp;
  }
  | AddExp ADDOP MulExp {
      auto exp = new AddExpAST();
      exp->addExp = BaseASTPtr($1);
      exp->op = *unique_ptr<string>($2);
      exp->subExp = BaseASTPtr($3);
      $$ = exp;
  }
  ;

RelExp
  : AddExp {
      auto exp = new RelExpAST();
      exp->subExp = BaseASTPtr($1);
      $$ = exp;
  }
  | RelExp RELOP AddExp {
      auto exp = new RelExpAST();
      exp->relExp = BaseASTPtr($1);
      exp->op = *unique_ptr<string>($2);
      exp->subExp = BaseASTPtr($3);
      $$ = exp;
  }
  ;

EqExp
    : RelExp {
        auto exp = new EqExpAST();
        exp->subExp = BaseASTPtr($1);
        $$ = exp;
    }
    | EqExp EQOP RelExp {
        auto exp = new EqExpAST();
        exp->eqExp = BaseASTPtr($1);
        exp->op = *unique_ptr<string>($2);
        exp->subExp = BaseASTPtr($3);
        $$ = exp;
    }
    ;

LAndExp
    : EqExp {
        auto exp = new LAndExpAST();
        exp->subExp = BaseASTPtr($1);
        $$ = exp;
    }
    | LAndExp LANDOP EqExp {
        auto exp = new LAndExpAST();
        exp->lAndExp = BaseASTPtr($1);
        exp->op = *unique_ptr<string>($2);
        exp->subExp = BaseASTPtr($3);
        $$ = exp;
    }
    ;

LOrExp
    : LAndExp {
        auto exp = new LOrExpAST();
        exp->subExp = BaseASTPtr($1);
        $$ = exp;
    }
    | LOrExp LOROP LAndExp {
        auto exp = new LOrExpAST();
        exp->lOrExp = BaseASTPtr($1);
        exp->op = *unique_ptr<string>($2);
        exp->subExp = BaseASTPtr($3);
        $$ = exp;
    }
    ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(BaseASTPtr &ast, const char *s) {
    fprintf(stderr, "ERROR\n");
}
