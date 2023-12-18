#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include "koopa.h"
#include "AST.hpp"
#include "riskv.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    string mode = (string)argv[1];
    auto input = argv[2];
    auto output = argv[4];

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);
    // output redirection
    freopen(output, "w", stdout);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    // For test: dump AST
    if(mode == "-test"){
        ast->Dump();
    }
    // Output Koopa IR
    else if(mode == "-koopa"){
        ast->DumpIR(); 
    }
    // Output RISC-V
    else{
        // Redirect STDOUT to a string and output Koopa IR to the string
        // Ref: https://blog.csdn.net/capsnever/article/details/129931660 (How to redirect STDOUT to a string)
        
        // Redirect STDOUT to a stringstream
        stringstream ss;
        streambuf *buf = cout.rdbuf();
        cout.rdbuf (ss.rdbuf());

        // Output Koopa IR to the string
        ast->DumpIR();
        string str = ss.str();
        const char * IRStr = str.data();

        // Reset STDOUT buffer
        cout.rdbuf(buf);
        // Generate raw program
        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_string(IRStr, &program);
        assert(ret == KOOPA_EC_SUCCESS); 
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        
        // Handle raw program
        Visit(raw);

        koopa_delete_program(program);
        
        // Release allocated memory
        koopa_delete_raw_program_builder(builder);
    }

    return 0;
}
