#include "../Transformer.h"

using namespace snowball::utils;
using namespace snowball::Syntax::transform;

namespace snowball {
namespace Syntax {

std::shared_ptr<types::DefinedType>
Transformer::transformClass(const std::string& uuid,
                            cacheComponents::Types::TypeStore& classStore,
                            Expression::TypeRef *typeRef) {
    auto ty = utils::cast<Statement::ClassDef>(classStore.type);
    assert(ty);

    // These are the generics generated outside of the class context.
    // for example, this "Test" type woudn't be fetched inside the class
    // context:
    //
    //   class Hello<T> { ... }
    //   Hello<?Test> // Test is not being transformed from the "Hello context".
    //
    // Note that the default class generics WILL be generated inside the class
    // context.
    auto generics = typeRef != nullptr
                        ? vector_iterate<Expression::TypeRef *,
                                         std::shared_ptr<types::Type>>(
                              typeRef->getGenerics(),
                              [&](auto t) { return transformType(t); })
                        : std::vector<std::shared_ptr<types::Type>>{};

    // TODO: check if typeRef generics match class generics
    std::shared_ptr<types::DefinedType> transformedType;
    ctx->withState(classStore.state, [&]() {
        ctx->withScope([&] {
            auto backupClass = ctx->getCurrentClass();

            // TODO: maybe not reset completly, add nested classes in the future
            ctx->setCurrentClass(nullptr);

            auto classGenerics = ty->getGenerics();

            // Fill out the remaining non-required tempalte parameters
            if (classGenerics.size() > generics.size()) {
                for (auto i = generics.size(); i < classGenerics.size(); ++i) {
                    generics.push_back(transformType(classGenerics[i]->type));
                }
            }

            for (int genericCount = 0; genericCount < generics.size();
                 genericCount++) {
                auto generic = classGenerics.at(genericCount);
                auto item    = std::make_shared<transform::Item>(
                    generics.at(genericCount));
                // TODO:
                // item->setDBGInfo(generic->getDBGInfo());
                ctx->addItem(generic->getName(), item);
            }

            std::shared_ptr<types::DefinedType> parentType = nullptr;
            if (auto x = ty->getParent()) {
                // TODO: check if it's actually a defined type
                parentType =
                    utils::dyn_cast<types::DefinedType>(transformType(x));
            }

            auto basedName  = getNameWithBase(ty->getName());
            auto baseFields = vector_iterate<
                Statement::VariableDecl *,
                types::DefinedType::ClassField
                    *>(ty->getVariables(), [&](auto v) {
                auto varTy = transformType(v->getDefinedType());
                return new types::DefinedType::ClassField(
                    v->getName(), varTy,
                    /*TODO: actually check this*/ Statement::Privacy::PRIVATE,
                    v->isMutable());
            });

            auto fields =
                getMemberList(ty->getVariables(), baseFields, parentType);

            auto baseUuid      = ctx->createIdentifierName(ty->getName());
            auto existantTypes = ctx->cache->getTransformedType(uuid);

            auto _uuid =
                baseUuid + ":" +
                utils::itos(existantTypes.has_value() ? existantTypes->size()
                                                      : 0);
            transformedType = std::make_shared<types::DefinedType>(
                basedName, _uuid, ctx->module, fields, parentType, generics);

            transformedType->setDBGInfo(ty->getDBGInfo());
            transformedType->setSourceInfo(ty->getSourceInfo());
            ctx->setCurrentClass(transformedType);

            auto item = std::make_shared<transform::Item>(transformedType);
            ctx->cache->setTransformedType(_uuid, item);

            for (auto fn : ty->getFunctions()) {
                fn->accept(this);
            }

            {
                // Set the default '=' operator for the class
                auto fn = std::make_shared<ir::Func>(
                    services::OperatorService::getOperatorMangle(
                        services::OperatorService::EQ),
                    true, false);
                auto arg      = std::make_shared<ir::Argument>("other");
                auto typeArgs = std::vector<std::shared_ptr<types::Type>>{
                    transformedType, transformedType};
                auto type = std::make_shared<types::FunctionType>(
                    typeArgs, transformedType);

                arg->setType(transformedType);

                fn->setArgs({{"other", arg}});
                fn->setType(type);
                fn->setPrivacy(/* public */ false);

                /// @see Transformer::defineFunction
                auto name = ctx->createIdentifierName(fn->getName(true));
                auto item = ctx->cache->getTransformedFunction(name);
                if (item) {
                    assert((*item)->isFunc());
                    (*item)->addFunction(fn);
                } else {
                    auto i = std::make_shared<transform::Item>(fn);
                    ctx->cache->setTransformedFunction(name, i);
                }
            }

            ctx->setCurrentClass(backupClass);
        });
    });

    return transformedType;
}

} // namespace Syntax
} // namespace snowball