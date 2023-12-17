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
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
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
%type <mul_val> BlockItems ConstDefs VarDefs // FuncFParams FuncRParams
%type <int_val> Number
%type <str_val> Type LVal

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// Note: $n = the nth parameter
FuncDef
  : Type IDENT '(' ')' Block {
    auto func_def = new FuncDefAST();
    func_def->func_type = *unique_ptr<string>($1); // Type
    func_def->ident = *unique_ptr<string>($2); // IDENT
    func_def->block = unique_ptr<BaseAST>($5); // Block
    $$ = func_def;
  }
  ;

Type
  : INT {
    $$ = new string("int");
  }
  | VOID {
        string *type = new string("void");
        $$ = type;
  }
  ;

Decl
    : ConstDecl {
        auto decl = new DeclAST();
        decl->def = DeclAST::def_const;
        decl->decl = unique_ptr<BaseAST>($1);
        $$ = decl;
    }
    | VarDecl {
        auto decl = new DeclAST();
        decl->def = DeclAST::def_var;
        decl->decl = unique_ptr<BaseAST>($1);
        $$ = decl;
    }
    ;

ConstDecl
  : CONST Type ConstDefs ';' {
    auto const_decl = new ConstDeclAST();
    const_decl->bType = *unique_ptr<string>($2);
    MulVecType *tmp = ($3);
    for (auto it = tmp->begin(); it != tmp->end(); it++)
      const_decl->constDefs.push_back(move(*it));
    $$ = const_decl;
  }
  ;

ConstDefs
  : ConstDef {
    auto const_defs = new MulVecType;
    const_defs->push_back(unique_ptr<BaseAST>($1));
    $$ = const_defs;
  }
  | ConstDefs ',' ConstDef {
    MulVecType *const_defs = ($1);
    const_defs->push_back(unique_ptr<BaseAST>($3));
    $$ = const_defs;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
      auto const_def = new ConstDefAST();
      const_def->ident = *unique_ptr<string>($1);
      const_def->constInitVal = unique_ptr<BaseAST>($3);
      $$ = const_def;
  }
  ;

ConstInitVal
  : ConstExp {
      auto const_init_val = new ConstInitValAST();
      // const_init_val->def = ConstInitValType::const_exp;
      const_init_val->subExp = unique_ptr<BaseAST>($1);
      $$ = const_init_val;
  }
  ;

VarDecl
  : Type VarDefs ';' {
    auto var_decl = new VarDeclAST();
    var_decl->bType = *unique_ptr<string>($1);
    MulVecType *tmp = ($2);
    for (auto it = tmp->begin(); it != tmp->end(); it++)
        var_decl->varDefs.push_back(move(*it));
    $$ = var_decl;
  }
  ;

VarDefs
  : VarDef {
    auto var_defs = new MulVecType;
    var_defs->push_back(unique_ptr<BaseAST>($1));
    $$ = var_defs;
  }
  | VarDefs ',' VarDef {
    MulVecType *var_defs = ($1);
    var_defs->push_back(unique_ptr<BaseAST>($3));
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
    var_def->initVal = unique_ptr<BaseAST>($3);
    $$ = var_def;
  }
  ;

InitVal
  : Exp {
    auto init_val = new InitValAST();
    // init_val->def = InitValAST::def_exp;
    init_val->subExp = unique_ptr<BaseAST>($1);
    $$ = init_val;
  }
  ;

Block
  : '{' BlockItems '}' {
    auto block = new BlockAST();
    MulVecType *tmp = ($2);
    for (auto it = tmp->begin(); it != tmp->end(); it++)
      block->blockItems.push_back(move(*it));
    $$ = block;
  }
  ;

BlockItems
  : {
    auto block_items = new MulVecType;
    $$ = block_items;
  }
  | BlockItems BlockItem {
    MulVecType *block_items = ($1);
    block_items->push_back(unique_ptr<BaseAST>($2));
    $$ = block_items;
  }
  ;

BlockItem
  : Decl {
    auto block_item = new BlockItemAST();
    block_item->def = BlockItemAST::def_decl;
    block_item->blockItem = unique_ptr<BaseAST>($1);
    $$ = block_item;
  }
  | ComplexStmt {
    auto block_item = new BlockItemAST();
    block_item->def = BlockItemAST::def_stmt;
    block_item->blockItem = unique_ptr<BaseAST>($1);
    $$ = block_item;  }
  ;

ComplexStmt
    : OpenStmt {
        auto stmt = ($1);
        $$ = stmt;
    }
    | ClosedStmt {
        auto stmt = ($1);
        $$ = stmt;
    }
    ;

ClosedStmt
  : Stmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_simple;
    stmt->subExp = unique_ptr<BaseAST>($1);
    $$ = stmt;
  }
  | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_ifelse;
    stmt->subExp = unique_ptr<BaseAST>($3);
    stmt->subStmt = unique_ptr<BaseAST>($5);
    stmt->elseStmt = unique_ptr<BaseAST>($7);
    $$ = stmt;
  }
  ;

OpenStmt
  : IF '(' Exp ')' ComplexStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_openif;
    stmt->subExp = unique_ptr<BaseAST>($3);
    stmt->subStmt = unique_ptr<BaseAST>($5);
    $$ = stmt;
  }
  | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto stmt = new ComplexStmtAST();
    stmt->def = ComplexStmtAST::def_ifelse;
    stmt->subExp = unique_ptr<BaseAST>($3);
    stmt->subStmt = unique_ptr<BaseAST>($5);
    stmt->elseStmt = unique_ptr<BaseAST>($7);
    $$ = stmt;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_ret;
    stmt->subExp = unique_ptr<BaseAST>($2);
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
    stmt->subExp = unique_ptr<BaseAST>($3);
    $$ = stmt;
  }
  | Exp ';' {
    auto stmt = new StmtAST();
    stmt->def = StmtAST::def_exp;
    stmt->subExp = unique_ptr<BaseAST>($1);
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
    stmt->subExp = unique_ptr<BaseAST>($1);
    $$ = stmt;
  };

LVal
  : IDENT {
    string *lval = new string(*unique_ptr<string>($1));
    $$ = lval;
  }
  ;

ConstExp 
  : Exp {
    auto const_exp = new ConstExpAST();
    const_exp->subExp = unique_ptr<BaseAST>($1);
    $$ = const_exp;
  }
  ;

Exp
  : LOrExp {
      auto exp = new ExpAST();
      exp->subExp = unique_ptr<BaseAST>($1);
      $$ = exp;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->def = PrimaryExpAST::def_bracketexp;
    primary_exp->subExp = unique_ptr<BaseAST>($2);
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
    unary_exp->subExp = unique_ptr<BaseAST>($1);
    $$ = unary_exp;
  }
  | ADDOP UnaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_unaryexp;
    unary_exp->op = *unique_ptr<string>($1);
    unary_exp->subExp = unique_ptr<BaseAST>($2);
    $$ = unary_exp;
  }
  | UNARYOP UnaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->def = UnaryExpAST::def_unaryexp;
    unary_exp->op = *unique_ptr<string>($1);
    unary_exp->subExp = unique_ptr<BaseAST>($2);
    $$ = unary_exp;
  }
  ;

MulExp
  : UnaryExp {
      auto exp = new MulExpAST();
      exp->subExp = unique_ptr<BaseAST>($1);
      $$ = exp;
  }
  | MulExp MULOP UnaryExp {
      auto exp = new MulExpAST();
      exp->mulExp = unique_ptr<BaseAST>($1);
      exp->op = *unique_ptr<string>($2);
      exp->subExp = unique_ptr<BaseAST>($3);
      $$ = exp;
  }
  ;

AddExp
  : MulExp {
      auto exp = new AddExpAST();
      exp->subExp = unique_ptr<BaseAST>($1);
      $$ = exp;
  }
  | AddExp ADDOP MulExp {
      auto exp = new AddExpAST();
      exp->addExp = unique_ptr<BaseAST>($1);
      exp->op = *unique_ptr<string>($2);
      exp->subExp = unique_ptr<BaseAST>($3);
      $$ = exp;
  }
  ;

RelExp
  : AddExp {
      auto exp = new RelExpAST();
      exp->subExp = unique_ptr<BaseAST>($1);
      $$ = exp;
  }
  | RelExp RELOP AddExp {
      auto exp = new RelExpAST();
      exp->relExp = unique_ptr<BaseAST>($1);
      exp->op = *unique_ptr<string>($2);
      exp->subExp = unique_ptr<BaseAST>($3);
      $$ = exp;
  }
  ;

EqExp
    : RelExp {
        auto exp = new EqExpAST();
        exp->subExp = unique_ptr<BaseAST>($1);
        $$ = exp;
    }
    | EqExp EQOP RelExp {
        auto exp = new EqExpAST();
        exp->eqExp = unique_ptr<BaseAST>($1);
        exp->op = *unique_ptr<string>($2);
        exp->subExp = unique_ptr<BaseAST>($3);
        $$ = exp;
    }
    ;

LAndExp
    : EqExp {
        auto exp = new LAndExpAST();
        exp->subExp = unique_ptr<BaseAST>($1);
        $$ = exp;
    }
    | LAndExp LANDOP EqExp {
        auto exp = new LAndExpAST();
        exp->lAndExp = unique_ptr<BaseAST>($1);
        exp->op = *unique_ptr<string>($2);
        exp->subExp = unique_ptr<BaseAST>($3);
        $$ = exp;
    }
    ;

LOrExp
    : LAndExp {
        auto exp = new LOrExpAST();
        exp->subExp = unique_ptr<BaseAST>($1);
        $$ = exp;
    }
    | LOrExp LOROP LAndExp {
        auto exp = new LOrExpAST();
        exp->lOrExp = unique_ptr<BaseAST>($1);
        exp->op = *unique_ptr<string>($2);
        exp->subExp = unique_ptr<BaseAST>($3);
        $$ = exp;
    }
    ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  extern int yylineno;    // defined and maintained in lex
    extern char *yytext;    // defined and maintained in lex
    int len=strlen(yytext);
    int i;
    char buf[512]={0};
    for (i=0;i<len;++i)
    {
        sprintf(buf,"%s%d", buf, yytext[i]);
    }
    fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}
