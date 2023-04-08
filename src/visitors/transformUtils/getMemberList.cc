#include "../Transformer.h"
#include "../../ast/types/DefinedType.h"

using namespace snowball::utils;
using namespace snowball::Syntax::transform;

namespace snowball {
namespace Syntax {

std::vector<ptr<types::DefinedType::ClassField>>
Transformer::getMemberList(std::vector<ptr<Syntax::Statement::VariableDecl>> fieldNodes,
    std::vector<ptr<types::DefinedType::ClassField>> fields,
    std::shared_ptr<types::DefinedType> parent) {
    std::vector<ptr<types::DefinedType::ClassField>> member_list;
    assert(fields.size() == fieldNodes.size());

    // add parent members first
    if (parent) {
        member_list.insert(member_list.end(), parent->getFields().begin(), parent->getFields().end());
    }

    // add current class members
    for (int i = 0; i < fields.size(); i++) {
        auto field = fields.at(i);
        // check if field has the same name as any member in member_list
        bool exists = false;
        for (const auto& member : member_list) {
            if (field->name == member->name) {
                exists = true;
                if (!field->type->is(member->type)) {
                    E<TYPE_ERROR>(fieldNodes.at(i), "Member with the same name '" + field->name +
                                             "' exists in parent with a different type.");
                }
                break;
            }
        }
        if (!exists) {
            member_list.push_back(field);
        }
    }

    return member_list;
}

} // namespace Syntax
} // namespace snowball