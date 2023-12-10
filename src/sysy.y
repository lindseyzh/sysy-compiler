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
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT UNARYOP MULOP ADDOP RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt 
%type <ast_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <int_val> Number
%type <str_val> FuncType

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
  : FuncType IDENT '(' ')' Block {
    auto func_def = new FuncDefAST();
    func_def->func_type = *unique_ptr<string>($1); // FuncType
    // func_def->func_type = *unique_ptr<BaseAST>($1); // FuncType
    func_def->ident = *unique_ptr<string>($2); // IDENT
    func_def->block = unique_ptr<BaseAST>($5); // Block
    $$ = func_def;
  }
  ;

FuncType
  : INT {
    $$ = new string("int");
  }
  ;

Block
  : '{' Stmt '}' {
    auto block = new BlockAST();
    block->stmt = unique_ptr<BaseAST>($2); // Stmt
    $$ = block;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto stmt = new StmtAST();
    stmt->subExp = unique_ptr<BaseAST>($2);
    $$ = stmt;
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
  | Number {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->def = PrimaryExpAST::def_numberexp;
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
