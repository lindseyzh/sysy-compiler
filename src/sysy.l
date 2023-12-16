%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>
#include "sysy.tab.hpp"
#include "AST.hpp"

using namespace std;

%}

/* White space & comments */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  "/*"([^*]*|(\*+[^/]))*"*/"

/* Identifier */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* Integer */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* Operator */
UnaryOperator \!  
/* We handle unary operators '+' and '-' as binary operator. See sysy.y for more detail. */
MulOperator   \*|\/|\%
AddOperator   \+|\-
RelOperator   <|>|<=|>=
EqOperator    ==|\!=
LAndOperator  &&
LOrOperator   \|\|


%%

{WhiteSpace}    { /* do nothing */ }
{LineComment}   { /* do nothing */ }
{BlockComment}  { /* do nothing */ }

"int"           { return INT; }
"return"        { return RETURN; }
"const"         { return CONST; }
"void"          { return VOID; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

{UnaryOperator}      { yylval.str_val = new string(yytext); return UNARYOP; }
{MulOperator}      { yylval.str_val = new string(yytext); return MULOP; }
{AddOperator}      { yylval.str_val = new string(yytext); return ADDOP; }
{RelOperator}      { yylval.str_val = new string(yytext); return RELOP; }
{EqOperator}      { yylval.str_val = new string(yytext); return EQOP; }
{LAndOperator}      { yylval.str_val = new string(yytext); return LANDOP; }
{LOrOperator}      { yylval.str_val = new string(yytext); return LOROP; }

.               { return yytext[0]; }

%%
