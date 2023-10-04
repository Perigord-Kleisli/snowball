#ifndef __SNOWBALL_LLVM_IR_CHUNK_H_
#define __SNOWBALL_LLVM_IR_CHUNK_H_

// we use it to avoid include loops
namespace snowball {
namespace Syntax {

/// @brief A node representing a chunk of the LLVM IR block.
struct LLVMIRChunk {
  enum { LLCode, TypeAccess } type;
  std::string code;
  types::Type* ty;
};

}
} // namespace snowball

#endif // __SNOWBALL_LLVM_IR_CHUNK_H_