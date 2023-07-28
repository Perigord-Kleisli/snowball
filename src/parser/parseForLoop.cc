
#include "../common.h"
#include "../lexer/tokens/token.h"
#include "../services/OperatorService.h"
#include "./Parser.h"

#include <assert.h>

namespace snowball::parser {

Syntax::Statement::ForLoop* Parser::parseForLoop() {
    assert(is<TokenType::KWORD_FOR>());
    auto dbg = DBGSourceInfo::fromToken(m_source_info, m_current);
    next(); // Eat "for"

    auto var = assert_tok<TokenType::IDENTIFIER>("identifier").to_string();
    next();
    assert_tok<TokenType::SYM_COLLON>("':'");
    auto expr = parseExpr();
    next();
    assert_tok<TokenType::BRACKET_LCURLY>("'{'");
    auto body = parseBlock();
    auto loop = Syntax::N<Syntax::Statement::ForLoop>(var, expr, body);
    loop->setDBGInfo(dbg);
    return loop;
}

} // namespace snowball::parser