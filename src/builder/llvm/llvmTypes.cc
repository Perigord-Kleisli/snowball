
#include "../../ast/errors/error.h"
#include "../../utils/utils.h"
#include "LLVMBuilder.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

using namespace snowball::utils;

namespace snowball {
namespace codegen {

llvm::Type* LLVMBuilder::getLLVMType(types::Type* t, bool translateVoid) {
  if (auto x = cast<types::IntType>(t)) {
    return builder->getIntNTy(x->getBits());
  } else if (auto x = cast<types::FloatType>(t)) {
    switch (x->getBits()) {
    case 16:
      return builder->getHalfTy();
    case 32:
      return builder->getFloatTy();
    case 64:
      return builder->getDoubleTy();
    default:
      assert(!"Unreachable type case found!");
    }
  } else if (cast<types::VoidType>(t)) {
    return translateVoid ? builder->getInt8Ty() : builder->getVoidTy();
  } else if (cast<types::CharType>(t)) {
    return builder->getInt8Ty();
  } else if (auto x = cast<types::ReferenceType>(t)) {
    return getLLVMType(x->getPointedType())->getPointerTo();
  } else if (auto x = cast<types::PointerType>(t)) {
    if (is<types::VoidType>(x->getPointedType()))
      return builder->getInt8PtrTy();
    return getLLVMType(x->getPointedType())->getPointerTo();
  } else if (auto f = cast<types::FunctionType>(t)) {
    return getLLVMFunctionType(f)->getPointerTo();
  } else if (auto a = cast<types::TypeAlias>(t)) {
    assert(!"Unreachable type case found!");
    return getLLVMType(a->getBaseType());
  } else if (auto c = cast<types::DefinedType>(t)) {
    llvm::StructType* s;
    if (auto it = types.find(c->getId()); it != types.end()) {
      return it->second;
    } else {
      s = llvm::StructType::create(
              *context, (c->isStruct() ? _SN_STRUCT_PREFIX : _SN_CLASS_PREFIX) + c->getMangledName());
      types.insert({c->getId(), s});
      assert(ctx->typeInfo.find(c->getId()) != ctx->typeInfo.end());
      c = ctx->typeInfo.find(c->getId())->second.get();
    }
    auto fields = c->getFields();
    auto generatedFields = vector_iterate<types::DefinedType::ClassField*, llvm::Type*>(
            fields, [&](types::DefinedType::ClassField* t) { return getLLVMType(t->type); });
    if (c->hasVtable) {
      auto t = getVtableType(c); // generate vtable type
      generatedFields.insert(generatedFields.begin(), llvm::FunctionType::get(
              builder->getInt32Ty(), {}, true
      )->getPointerTo()->getPointerTo());
    }
    s->setBody(generatedFields);
    return s;
  } else {
    Syntax::E<BUG>(FMT("Undefined type! ('%s')", t->getName().c_str()));
  }

  assert(false);
  return nullptr; // to avoid warnings
}

llvm::FunctionType* LLVMBuilder::getLLVMFunctionType(types::FunctionType* fn) {
  auto argTypes =
          vector_iterate<types::Type*, llvm::Type*>(fn->getArgs(), [&](types::Type* arg) { return getLLVMType(arg); });

  auto ret = getLLVMType(fn->getRetType());
  if (utils::is<types::DefinedType>(fn->getRetType())) {
    argTypes.insert(argTypes.begin(), ret->getPointerTo());
    ret = builder->getVoidTy();
  }

  return llvm::FunctionType::get(ret, argTypes, fn->isVariadic());
}

} // namespace codegen
} // namespace snowball
