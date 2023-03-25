
#include "../../ir/values/ValueExtract.h"
#include "../../utils/utils.h"
#include "LLVMBuilder.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

namespace snowball {
namespace codegen {

void LLVMBuilder::visit(ptr<ir::ValueExtract> extract) {
    auto var = extract->getValue();

    ptr<llvm::Value> value = nullptr;
    if (auto f = std::dynamic_pointer_cast<ir::Func>(var)) {
        auto fn = funcs.at(f->getId());
        value   = fn;
    } else if (auto v = std::dynamic_pointer_cast<ir::Variable>(var)) {
        // note(argument): "x + 1" because ir::Argument (x - 1) gets created
        // after ir::Variable (x). note(note argument): They are declared as
        // usual with normal ID incrementation
        auto id = v->getId() + (v->isArgument());
        value   = ctx->getSymbol(id);
    } else {
        assert(false && "BUG: Value extract type not supported!");
    }

    this->value = builder->CreateLoad(getLLVMType(var->getType()), value);
}

} // namespace codegen
} // namespace snowball
