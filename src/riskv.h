#pragma once
#include<iostream>
#include<string>
#include "koopa.h"

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
// Reg Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
// Reg Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_integer_t &integer);

// Visit a raw program
void Visit(const koopa_raw_program_t &program) {
  // Note: "values" and "funcs" are both "koopa_raw_slice_t" type
  Visit(program.values);

  std::cout << "\t.text" << "\n";
  Visit(program.funcs);
}

// Visit a raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        assert(false);
    }
  }
}

// Visit a function
void Visit(const koopa_raw_function_t &func) {
  std::cout << "\t.globl " << func->name + 1 << "\n";
  if(func->bbs.len == 0) return;
  std::cout << func->name + 1 << ":\n";
  for (size_t j = 0; j < func->bbs.len; ++j) {
    assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];
  }
  Visit(func->bbs);
}

// Visit a basic block
void Visit(const koopa_raw_basic_block_t &bb) {
  Visit(bb->insts);
}

// Visit an instruction
void Visit(const koopa_raw_value_t &value) {
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // return
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // integer
      Visit(kind.data.integer);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

void Visit(const koopa_raw_return_t &ret){
    std::cout << "\tli a0, " << ret.value << "\n";
    std::cout << "ret\n";
    return;
}

void Visit(const koopa_raw_integer_t &integer){

}


