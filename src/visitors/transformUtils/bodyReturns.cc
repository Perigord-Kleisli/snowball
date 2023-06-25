#include "../Transformer.h"

using namespace snowball::utils;
using namespace snowball::Syntax::transform;

namespace snowball {
namespace Syntax {

bool Transformer::bodyReturns(std::vector<Node*> exprs) {
    for (auto expr : exprs) {
        if (cast<Statement::Return>(expr)) {
            return true;
        } else if (auto b = cast<Block>(expr)) {
            if (bodyReturns(b->getStmts())) return true;
        } else if (auto c = cast<Statement::Conditional>(expr)) {
            auto elseBlock = c->getElse();
            if (bodyReturns(c->getBlock()->getStmts()) && elseBlock &&
                bodyReturns(elseBlock->getStmts())) {
                return true;
            }
        }
        // TODO: Break, continue: return false
        // TODO: Loop, while: if (bodyReturns(b->getStmts())) return
        // true;

        // Ignore unhandled!
    }

    return false;
}

} // namespace Syntax
} // namespace snowball