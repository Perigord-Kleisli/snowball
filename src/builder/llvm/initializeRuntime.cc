
#include "../../utils/utils.h"
#include "../../../runtime/libs/runtime.h"
#include "LLVMBuilder.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace snowball {
namespace codegen {

void LLVMBuilder::initializeRuntime() {
  auto ty = llvm::FunctionType::get(builder->getVoidTy(), {builder->getInt32Ty()}, false);
  auto f = llvm::cast<llvm::Function>(
          module->getOrInsertFunction(getSharedLibraryName("sn.runtime.initialize"), ty).getCallee()
  );
  f->addFnAttr(llvm::Attribute::AlwaysInline);
  f->addFnAttr(llvm::Attribute::NoUnwind);
  const int flags = (dbg.debug ? SNOWBALL_FLAG_DEBUG : 0) | 0; 
  auto mainFunction = module->getFunction(_SNOWBALL_FUNCTION_ENTRY);
  bool buildReturn = false;
  llvm::BasicBlock* body;
  if (mainFunction) {
    if (ctx->testMode) {
      mainFunction->eraseFromParent();
      auto fnType = llvm::FunctionType::get(builder->getInt32Ty(), {});
      mainFunction = (llvm::Function*) module->getOrInsertFunction(_SNOWBALL_FUNCTION_ENTRY, fnType).getCallee();
      setPersonalityFunction(mainFunction);
      body = llvm::BasicBlock::Create(builder->getContext(), "test_entry", mainFunction);
    } else if (ctx->benchmarkMode) {
      mainFunction->eraseFromParent();
      auto fnType = llvm::FunctionType::get(builder->getInt32Ty(), {});
      mainFunction = (llvm::Function*) module->getOrInsertFunction(_SNOWBALL_FUNCTION_ENTRY, fnType).getCallee();
      setPersonalityFunction(mainFunction);
      body = llvm::BasicBlock::Create(builder->getContext(), "benchmark_entry", mainFunction);
    } else {
      body = &mainFunction->front();
    }
  } else {
    auto fnType = llvm::FunctionType::get(builder->getInt32Ty(), {});
    mainFunction = (llvm::Function*) module->getOrInsertFunction(_SNOWBALL_FUNCTION_ENTRY, fnType).getCallee();
    setPersonalityFunction(mainFunction);
    body = llvm::BasicBlock::Create(builder->getContext(), "entry", mainFunction);
    buildReturn = !ctx->testMode;
  }

  builder->SetInsertPoint(body);
  auto flagsInt = builder->getInt32(flags);
  if (buildReturn) {
    builder->CreateCall(f, {flagsInt});
    builder->CreateRet(builder->getInt32(0));
  } else if (ctx->testMode) {
    builder->CreateCall(f, {flagsInt});
    createTests(mainFunction);
  } else if (ctx->benchmarkMode) {
    builder->CreateCall(f, {flagsInt});
    createBenchmark(mainFunction);
  } else {
    llvm::CallInst::Create(f, {}, "", &body->front());
  }
}

} // namespace codegen
} // namespace snowball
