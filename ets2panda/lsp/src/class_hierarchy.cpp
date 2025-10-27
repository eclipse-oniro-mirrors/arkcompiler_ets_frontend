/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "class_hierarchy.h"
#include <iostream>
#include <ostream>
#include "class_hierarchies.h"
#include "compiler/lowering/util.h"
#include "public/public.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/completions.h"

namespace ark::es2panda::lsp {

std::string GetHierarchyDeclarationFileName(const ir::AstNode *node)
{
    if (node == nullptr) {
        return "";
    }
    if (node->IsClassDeclaration()) {
        if (node->AsClassDeclaration()->Definition()->Ident()->Range().start.Program() == nullptr) {
            return "";
        }
        return std::string(node->AsClassDeclaration()->Definition()->Ident()->Range().start.Program()->AbsoluteName());
    }
    if (node->IsTSInterfaceDeclaration()) {
        if (node->AsTSInterfaceDeclaration()->Id()->Range().start.Program() == nullptr) {
            return "";
        }
        return std::string(node->AsTSInterfaceDeclaration()->Id()->Range().start.Program()->AbsoluteName());
    }
    return "";
}

std::string GetHierarchyDeclarationName(const ir::AstNode *node)
{
    if (node == nullptr) {
        return "";
    }
    if (node->IsClassDeclaration()) {
        return std::string(node->AsClassDeclaration()->Definition()->Ident()->Name());
    }
    if (node->IsTSInterfaceDeclaration()) {
        return std::string(node->AsTSInterfaceDeclaration()->Id()->Name());
    }
    return "";
}

HierarchyType GetHierarchyType(const ir::AstNode *node)
{
    if (node == nullptr) {
        return HierarchyType::OTHERS;
    }
    if (node->IsClassDeclaration()) {
        return HierarchyType::CLASS;
    }
    if (node->IsTSInterfaceDeclaration()) {
        return HierarchyType::INTERFACE;
    }
    return HierarchyType::OTHERS;
}

size_t GetPosition(const ir::AstNode *node)
{
    if (node == nullptr) {
        return 0;
    }
    if (node->IsClassDeclaration()) {
        return node->AsClassDeclaration()->Definition()->Ident()->Start().index;
    }
    if (node->IsTSInterfaceDeclaration()) {
        return node->AsTSInterfaceDeclaration()->Id()->Start().index;
    }
    return 0;
}

const ir::AstNode *GetEffectiveBaseTypeNode(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassDeclaration()) {
        return nullptr;
    }
    auto super = node->AsClassDeclaration()->Definition()->Super();
    if (super == nullptr || !super->IsETSTypeReference()) {
        return nullptr;
    }
    auto id = super->AsETSTypeReference()->Part()->Name();
    if (id == nullptr || !id->IsIdentifier()) {
        return nullptr;
    }
    auto result = compiler::DeclarationFromIdentifier(id->AsIdentifier());
    if (result == nullptr || !result->IsClassDefinition()) {
        return nullptr;
    }
    return result->Parent();
}

std::vector<const ir::AstNode *> GetInterfaceExtendsHeritageElement(const ir::AstNode *node)
{
    std::vector<const ir::AstNode *> result;
    if (node == nullptr || !node->IsTSInterfaceDeclaration()) {
        return result;
    }
    auto extends = node->AsTSInterfaceDeclaration()->Extends();
    for (auto e : extends) {
        auto id = e->Expr()->AsETSTypeReference()->Part()->Name();
        result.push_back(compiler::DeclarationFromIdentifier(id->AsIdentifier()));
    }
    return result;
}

void FindSuper(const ir::AstNode *node, TypeHierarchies &typeHierarchies, std::set<TypeHierarchies> &superLists)
{
    auto name = GetHierarchyDeclarationName(node);
    if (name.empty()) {
        return;
    }
    TypeHierarchies subOrSuper(GetHierarchyDeclarationFileName(node), name, GetHierarchyType(node), GetPosition(node));
    if (superLists.find(subOrSuper) != superLists.end()) {
        return;
    }
    superLists.insert(subOrSuper);
    GetSuperTypeHierarchies(node, subOrSuper, superLists);
    typeHierarchies.subOrSuper.emplace_back(subOrSuper);
}

std::vector<ir::AstNode *> GetEffectiveImplementsTypeNodes(const ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    if (node == nullptr || !node->IsClassDeclaration()) {
        return result;
    }
    auto implements = node->AsClassDeclaration()->Definition()->Implements();
    for (auto imp : implements) {
        result.emplace_back(compiler::DeclarationFromIdentifier(
            imp->AsTSClassImplements()->Expr()->AsETSTypeReference()->Part()->Name()->AsIdentifier()));
    }
    return result;
}

void GetSuperTypeHierarchies(const ir::AstNode *node, TypeHierarchies &typeHierarchies,
                             std::set<TypeHierarchies> &superLists)
{
    std::set<TypeHierarchies> currentList;
    auto extendsNode = GetEffectiveBaseTypeNode(node);
    if (extendsNode != nullptr) {
        currentList = superLists;
        FindSuper(extendsNode, typeHierarchies, currentList);
    }
    auto implementsNodes = GetEffectiveImplementsTypeNodes(node);
    for (auto n : implementsNodes) {
        currentList = superLists;
        FindSuper(n, typeHierarchies, currentList);
    }
    auto extendsNodes = GetInterfaceExtendsHeritageElement(node);
    for (auto n : extendsNodes) {
        currentList = superLists;
        FindSuper(n, typeHierarchies, currentList);
    }
}

ir::AstNode *GetCurrentClassOrInterfaceDeclaration(ir::AstNode *node)
{
    auto tmp = node;
    while (tmp != nullptr) {
        if (tmp->IsClassDeclaration() || tmp->IsTSInterfaceDeclaration()) {
            return tmp;
        }
        tmp = tmp->Parent();
    }
    return nullptr;
}

ir::AstNode *GetTargetDeclarationNodeByPosition(es2panda_Context *context, size_t pos)
{
    auto node = ark::es2panda::lsp::GetTouchingToken(context, pos, false);
    if (node == nullptr) {
        return nullptr;
    }
    return GetCurrentClassOrInterfaceDeclaration(node);
}

bool IsChildNode(const ir::AstNode *child, const ir::AstNode *parent)
{
    std::vector<const ir::AstNode *> parentList = GetInterfaceExtendsHeritageElement(child);
    auto baseNode = GetEffectiveBaseTypeNode(child);
    if (baseNode != nullptr) {
        parentList.emplace_back(baseNode);
    }
    auto parentName = GetHierarchyDeclarationName(parent);
    auto parentFileName = GetHierarchyDeclarationFileName(parent);
    auto parentPosition = GetPosition(parent);
    auto result = std::find_if(parentList.begin(), parentList.end(), [&](const ir::AstNode *n) {
        auto p = GetPosition(n);
        return parentName == GetHierarchyDeclarationName(n) && parentFileName == GetHierarchyDeclarationFileName(n) &&
               p == parentPosition;
    });
    return result != parentList.end();
}

std::vector<ir::AstNode *> GetImplementationReferenceEntries(es2panda_Context *context, const ir::AstNode *node,
                                                             std::set<TypeHierarchies> &subLists)
{
    std::vector<ir::AstNode *> result;
    if (context == nullptr) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return result;
    }
    auto astNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    astNode->IterateRecursively([&subLists, node, &result](ir::AstNode *child) {
        if (child == nullptr || (!child->IsClassDeclaration() && !child->IsTSInterfaceDeclaration())) {
            return;
        }
        auto name = GetHierarchyDeclarationName(child);
        if (name.empty()) {
            return;
        }
        auto fileName = GetHierarchyDeclarationFileName(child);
        if (fileName.empty()) {
            return;
        }
        TypeHierarchies childTypeHierarchies(fileName, name, GetHierarchyType(child), GetPosition(child));
        if (subLists.find(childTypeHierarchies) != subLists.end()) {
            return;
        }
        if (!IsChildNode(child, node)) {
            return;
        }
        result.emplace_back(child);
        subLists.insert(childTypeHierarchies);
    });
    return result;
}

void GetSubTypeHierarchies(es2panda_Context *context, const ir::AstNode *node, TypeHierarchies &typeHierarchies,
                           std::set<TypeHierarchies> &subLists)
{
    if (node == nullptr || (!node->IsTSInterfaceDeclaration() && !node->IsClassDeclaration())) {
        return;
    }
    auto name = GetHierarchyDeclarationName(node);
    if (name.empty()) {
        return;
    }
    auto childList = GetImplementationReferenceEntries(context, node, subLists);
    for (auto child : childList) {
        TypeHierarchies childType(GetHierarchyDeclarationFileName(child), GetHierarchyDeclarationName(child),
                                  GetHierarchyType(child), GetPosition(child));
        std::set<TypeHierarchies> curList;
        curList.insert(childType);
        GetSubTypeHierarchies(context, child, childType, curList);
        typeHierarchies.subOrSuper.emplace_back(childType);
    }
}

void InitHierarchies(TypeHierarchies &typeHierarchies, std::string &fileName, std::string &name, HierarchyType type,
                     size_t pos)
{
    typeHierarchies.fileName = fileName;
    typeHierarchies.name = name;
    typeHierarchies.type = type;
    typeHierarchies.pos = pos;
}

TypeHierarchiesInfo GetTypeHierarchiesImpl(es2panda_Context *context, size_t pos, const ir::AstNode *declaration)
{
    TypeHierarchiesInfo result;
    if (context == nullptr) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return result;
    }
    if (declaration == nullptr) {
        declaration = GetTargetDeclarationNodeByPosition(context, pos);
    }
    if (declaration == nullptr || (!declaration->IsTSInterfaceDeclaration() && !declaration->IsClassDeclaration())) {
        return result;
    }
    result.fileName = GetHierarchyDeclarationFileName(declaration);
    result.name = GetHierarchyDeclarationName(declaration);
    result.type = GetHierarchyType(declaration);
    result.pos = GetPosition(declaration);
    InitHierarchies(result.superHierarchies, result.fileName, result.name, result.type, result.pos);
    InitHierarchies(result.subHierarchies, result.fileName, result.name, result.type, result.pos);
    std::set<TypeHierarchies> superLists;
    superLists.insert(result.superHierarchies);
    GetSuperTypeHierarchies(declaration, result.superHierarchies, superLists);
    std::set<TypeHierarchies> subLists;
    subLists.insert(result.subHierarchies);
    GetSubTypeHierarchies(context, declaration, result.subHierarchies, subLists);
    return result;
}

/**
 * @brief Helper function to get directly implemented interfaces
 * @param node - Class definition node
 * @return Vector of directly implemented interface nodes
 */
std::vector<ir::AstNode *> GetImplements(ir::AstNode *node)
{
    std::vector<ir::AstNode *> result {};
    if (node == nullptr) {
        return result;
    }
    auto classDefinition = node->AsClassDefinition();
    auto implements = classDefinition->Implements();
    for (auto implement : implements) {
        auto partNode = GetIdentifierFromTSInterfaceHeritage(implement);
        if (partNode == nullptr || !partNode->IsIdentifier()) {
            continue;
        }
        auto interfaceDecl = compiler::DeclarationFromIdentifier(partNode->AsIdentifier());
        if (interfaceDecl && interfaceDecl->IsTSInterfaceDeclaration()) {
            result.push_back(interfaceDecl->AsTSInterfaceDeclaration());
        }
    }
    return result;
}

/**
 * @brief (查找当前类的子类，或者当前接口的子接口) Finds direct descendants (subclasses or subinterfaces) of
 * the given node
 * @param context - Compiler context containing program AST
 * @param node - Class or interface declaration node to find descendants for
 * @return Vector of direct descendant nodes (subclasses or subinterfaces)
 */
std::vector<ir::AstNode *> FindDirectDescendants(es2panda_Context *context, const ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    if (context == nullptr) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return result;
    }
    auto astNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    astNode->IterateRecursively([node, &result](ir::AstNode *child) {
        if (child == nullptr || (!child->IsClassDeclaration() && !child->IsTSInterfaceDeclaration())) {
            return;
        }
        auto name = GetHierarchyDeclarationName(child);
        if (name.empty()) {
            return;
        }
        auto fileName = GetHierarchyDeclarationFileName(child);
        if (fileName.empty()) {
            return;
        }
        if (!IsChildNode(child, node)) {
            return;
        }
        result.emplace_back(child);
    });
    return result;
}

/**
 * @brief (查找当前接口的实现类) Finds classes that directly implement the given interface
 * @param context - Compiler context containing program AST
 * @param node - Interface declaration node
 * @return Vector of class nodes that directly implement the interface
 */
std::vector<ir::AstNode *> FindDirectImplementingClasses(es2panda_Context *context, const ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    if (context == nullptr) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return result;
    }
    if (!node->IsTSInterfaceDeclaration()) {
        return result;
    }
    auto astNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    astNode->IterateRecursively([node, &result](ir::AstNode *child) {
        if (child == nullptr || !child->IsClassDeclaration()) {
            return;
        }
        auto name = GetHierarchyDeclarationName(child);
        if (name.empty()) {
            return;
        }
        auto fileName = GetHierarchyDeclarationFileName(child);
        if (fileName.empty()) {
            return;
        }
        auto impls = GetImplements(child->AsClassDeclaration()->Definition());
        for (auto impl : impls) {
            if (GetIdentifierName(const_cast<ir::AstNode *>(impl)) ==
                    GetIdentifierName(const_cast<ir::AstNode *>(node)) &&
                impl->Start().index == node->Start().index &&
                GetHierarchyDeclarationFileName(impl) == GetHierarchyDeclarationFileName(node)) {
                result.emplace_back(child);
                return;
            }
        }
    });
    return result;
}

/**
 *
 * @brief 如果当前node是接口类型，查找有哪些接口是继承或者间接的继承当前的接口；如果当前node是类，查找当前类的子类
 * 以及间接的实现类
 * Recursively collects all descendant nodes (subclasses/subinterfaces)
 * @param contextList - List of compiler contexts to search
 * @param node - Starting class/interface declaration node
 * @param result - Output vector to store all descendant nodes
 */
void CollectAllDescendants(std::vector<es2panda_Context *> *contextList, const ir::AstNode *node,
                           std::vector<ir::AstNode *> &result)
{
    for (auto context : *contextList) {
        auto rs = FindDirectDescendants(context, node);
        result.insert(result.end(), rs.begin(), rs.end());
        for (auto subNode : rs) {
            CollectAllDescendants(contextList, subNode, result);
        }
    }
}

/**
 * @brief 查找当前接口的实现类，或者间接的实现类
 * Recursively collects all implementing classes for an interface
 * @param contextList - List of compiler contexts to search
 * @param node - Starting interface declaration node
 * @param result - Output vector to store implementing classes
 */
void CollectAllImplementingClasses(std::vector<es2panda_Context *> *contextList, const ir::AstNode *node,
                                   std::vector<ir::AstNode *> &result)
{
    for (auto context : *contextList) {
        auto rs = FindDirectImplementingClasses(context, node);
        result.insert(result.end(), rs.begin(), rs.end());
        for (auto subNode : rs) {
            CollectAllDescendants(contextList, subNode, result);
        }
    }
}

/**
 * @brief (查找当前类的父类) Find immediate superclass of current class node
 * @param node - current class node declaration
 * @return Pointer to the direct superclass node or nullptr if not found
 */
ir::AstNode *GetClassDirectSuperClass(ir::AstNode *node)
{
    if (!node->IsClassDeclaration()) {
        return nullptr;
    }
    auto classNode = node->AsClassDeclaration()->Definition();
    auto super = classNode->Super();
    if (super == nullptr || !super->IsETSTypeReference()) {
        return nullptr;
    }
    auto part = super->AsETSTypeReference()->Part();
    if (part == nullptr || !part->IsETSTypeReferencePart()) {
        return nullptr;
    }
    auto partNode = part->AsETSTypeReferencePart()->Name();
    if (partNode == nullptr || !partNode->IsIdentifier()) {
        return nullptr;
    }
    auto superClass = compiler::DeclarationFromIdentifier(partNode->AsIdentifier());
    if (superClass && superClass->IsClassDefinition()) {
        return superClass->Parent();
    }
    return nullptr;
}

/**
 * @brief (1. 查找当前类的(所有)父类) Find all superclasses of the current class node
 * @param context - Compiler context (unused)
 * @param node - Current class declaration node
 * @return Vector of superclass nodes in inheritance order
 */
std::vector<ir::AstNode *> GetClassSuperClasses([[maybe_unused]] std::vector<es2panda_Context *> *contextList,
                                                ir::AstNode *node)
{
    std::vector<ir::AstNode *> res;

    ir::AstNode *subClass = node;
    ir::AstNode *superClass = nullptr;
    do {
        superClass = GetClassDirectSuperClass(subClass);
        if (superClass != nullptr) {
            res.push_back(superClass);
        }
        subClass = superClass;
    } while (superClass != nullptr);

    return res;
}

/**
 * @brief (2. 查找当前类的(所有)子类) Find all possible implementing classes of current class node
 * @param context - Compiler context containing program AST
 * @param node - Current class declaration node
 * @return Vector of all subclass nodes
 */
std::vector<ir::AstNode *> GetClassSubClasses(std::vector<es2panda_Context *> *contextList, ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    CollectAllDescendants(contextList, node, result);
    return result;
}

/**
 * @brief (查找当前类的父接口) Find interface of current class node
 * @param node - Current class declaration node
 * @return Set of directly implemented interface nodes
 */
std::unordered_set<ir::AstNode *> GetClassDirectImplementedInterfaces(ir::AstNode *node)
{
    std::unordered_set<ir::AstNode *> res;
    if (node == nullptr) {
        return res;
    }
    if (!node->IsClassDeclaration()) {
        return res;
    }
    auto classDefinition = node->AsClassDeclaration()->Definition();
    auto implements = classDefinition->Implements();
    for (auto implement : implements) {
        auto partNode = GetIdentifierFromTSInterfaceHeritage(implement);
        if (partNode == nullptr) {
            continue;
        }
        auto interfaceDecl = compiler::DeclarationFromIdentifier(partNode->AsIdentifier());
        if (interfaceDecl && interfaceDecl->IsTSInterfaceDeclaration()) {
            res.insert(interfaceDecl);
        }
    }
    return res;
}

/**
 * @brief (查找当前接口的父接口) Find which interfaces current interface node extends
 * @param node - Current declaration node
 * @return Set of directly extended interface nodes
 */
std::unordered_set<ir::AstNode *> GetInterfaceDirectExtendedInterfaces(ir::AstNode *node)
{
    std::unordered_set<ir::AstNode *> res;
    if (node == nullptr) {
        return res;
    }
    if (!node->IsTSInterfaceDeclaration()) {
        return res;
    }
    auto childInterface = node->AsTSInterfaceDeclaration();
    auto extends = childInterface->Extends();
    for (auto extend : extends) {
        auto partNode = GetIdentifierFromTSInterfaceHeritage(extend->AsTSInterfaceHeritage());
        if (partNode == nullptr || !partNode->IsIdentifier()) {
            continue;
        }
        auto interfaceDecl = compiler::DeclarationFromIdentifier(partNode->AsIdentifier());
        if (interfaceDecl && interfaceDecl->IsTSInterfaceDeclaration()) {
            res.insert(interfaceDecl);
        }
    }
    return res;
}

/**
 * @brief (3. 查找当前类的(所有)父接口) Find all interfaces extended by current class node
 * @param context - Compiler context (unused)
 * @param node - Current class declaration node
 * @return Vector of implemented interface nodes
 */
std::vector<ir::AstNode *> GetClassImplementedInterfaces([[maybe_unused]] std::vector<es2panda_Context *> *contextList,
                                                         ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    std::unordered_set<ir::AstNode *> resultSet;
    std::unordered_set<ir::AstNode *> subClasses {node};
    std::unordered_set<ir::AstNode *> subInterfaces;
    std::unordered_set<ir::AstNode *> gottenSet;
    std::unordered_set<ir::AstNode *> directSuperClasses;
    std::unordered_set<ir::AstNode *> directSuperInterfaces;
    do {
        directSuperInterfaces.clear();
        directSuperClasses.clear();
        for (auto subClass : subClasses) {
            if (gottenSet.find(subClass) != gottenSet.end()) {
                continue;
            }
            auto superInterfaces = GetClassDirectImplementedInterfaces(subClass);
            directSuperInterfaces.insert(superInterfaces.begin(), superInterfaces.end());
            auto superClass = GetClassDirectSuperClass(subClass);
            if (superClass != nullptr) {
                directSuperClasses.insert(superClass);
            }
        }
        gottenSet.insert(subClasses.begin(), subClasses.end());
        for (auto subInterface : subInterfaces) {
            if (gottenSet.find(subInterface) != gottenSet.end()) {
                continue;
            }
            auto superInterfaces = GetInterfaceDirectExtendedInterfaces(subInterface);
            directSuperInterfaces.insert(superInterfaces.begin(), superInterfaces.end());
        }
        gottenSet.insert(subInterfaces.begin(), subInterfaces.end());
        if (!directSuperInterfaces.empty()) {
            resultSet.insert(directSuperInterfaces.begin(), directSuperInterfaces.end());
            subInterfaces.clear();
            subInterfaces.insert(directSuperInterfaces.begin(), directSuperInterfaces.end());
        }
        if (!directSuperClasses.empty()) {
            subClasses.clear();
            subClasses.insert(directSuperClasses.begin(), directSuperClasses.end());
        }
    } while (!directSuperInterfaces.empty() || !directSuperClasses.empty());
    result.insert(result.end(), resultSet.begin(), resultSet.end());
    return result;
}

/**
 * @brief (4. 查找当前接口的(所有)父接口) Find all interfaces extended by current interface node
 * @param context - Compiler context (unused)
 * @param node - Current interface node
 * @return Vector of ancestor interface nodes
 */
std::vector<ir::AstNode *> GetInterfaceSuperInterfaces([[maybe_unused]] std::vector<es2panda_Context *> *contextList,
                                                       ir::AstNode *node)
{
    std::vector<ir::AstNode *> superInterfaces {};
    std::unordered_set<ir::AstNode *> visited;
    std::function<void(ir::AstNode *)> findSuperInterfaces = [&](ir::AstNode *currentNode) {
        if (currentNode == nullptr) {
            return;
        }
        if (!visited.insert(currentNode).second) {
            return;
        }
        auto extends = currentNode->AsTSInterfaceDeclaration()->Extends();
        for (auto extend : extends) {
            auto partNode = GetIdentifierFromTSInterfaceHeritage(extend);
            if (partNode == nullptr) {
                continue;
            }
            auto interfaceDecl = compiler::DeclarationFromIdentifier(partNode->AsIdentifier());
            if (interfaceDecl && interfaceDecl->IsTSInterfaceDeclaration()) {
                ir::AstNode *superInterface = interfaceDecl->AsTSInterfaceDeclaration();
                superInterfaces.push_back(superInterface);
                findSuperInterfaces(superInterface);
            }
        }
    };
    findSuperInterfaces(node);
    return superInterfaces;
}

/**
 * @brief (5. 查找当前接口的(所有)子接口) Find all interfaces extended by current interface node
 * @param context - Compiler context containing program AST
 * @param node - Current interface node
 * @return Vector of descendant interface nodes
 */
std::vector<ir::AstNode *> GetInterfaceSubInterfaces(std::vector<es2panda_Context *> *contextList, ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    CollectAllDescendants(contextList, node, result);
    return result;
}

/**
 * @brief (6. 查找当前接口的(所有)子类) Find all interfaces current interface node extends & their corresponding
 * implementation classes
 * @param context - Compiler context containing program AST
 * @param node - Current interface node
 * @return Vector of implementing class nodes
 */
std::vector<ir::AstNode *> GetInterfaceImplementingClasses(std::vector<es2panda_Context *> *contextList,
                                                           ir::AstNode *node)
{
    std::vector<ir::AstNode *> result;
    CollectAllImplementingClasses(contextList, node, result);
    return result;
}

/**
 * @brief Extracts public, non-static, non-constructor members from a class or interface node.
 * @param context Unused context pointer.
 * @param node    AST node (TSInterfaceDeclaration or ClassDeclaration).
 * @return Vector of AST nodes for class properties and filtered methods.
 */
std::vector<ir::AstNode *> GetMembers([[maybe_unused]] std::vector<es2panda_Context *> *contextList, ir::AstNode *node)
{
    std::vector<ir::AstNode *> res;
    std::vector<ir::AstNode *> body;
    if (node->IsTSInterfaceDeclaration()) {
        auto interfaceBody = node->AsTSInterfaceDeclaration()->Body()->Body();
        body.insert(body.end(), interfaceBody.begin(), interfaceBody.end());
    } else {
        if (node->IsClassDefinition()) {
            node = node->Parent();
        }
        auto classBody = node->AsClassDeclaration()->Definition()->Body();
        body.insert(body.end(), classBody.begin(), classBody.end());
    }
    for (auto *field : body) {
        if (field->IsClassProperty()) {
            res.emplace_back(field);
        } else if (field->IsMethodDefinition()) {
            auto *method = field->AsMethodDefinition();
            if (!method->IsPrivate() && !method->IsStatic() && !method->IsConstructor()) {
                res.emplace_back(field);
            }
        }
    }
    return res;
}

/**
 * @brief Determines if two method nodes have matching signatures
 * @param a - First method node
 * @param b - Second method node
 * @return True if methods have identical identifier names
 */
bool IsMethodMatch(ir::AstNode *a, ir::AstNode *b)
{
    return GetIdentifierName(a) == GetIdentifierName(b);
}

/**
 * @brief Compares member matches and records matched and unmatched items
 * @param currentMembers the list of members in the current class
 * @param targetMembers the list of members in the target class
 * @param matchedContainer the container to record matched members
 * @param unmatchedContainer the container to record unmatched members
 * @param fileName the file name, used for recording location information
 */
void CompareMembersCommon(const std::vector<ir::AstNode *> &currentMembers,
                          const std::vector<ir::AstNode *> &targetMembers,
                          std::vector<ClassRelationDetails> &matchedContainer,
                          std::vector<ClassRelationDetails> &unmatchedContainer,
                          [[maybe_unused]] const std::string &fileName)
{
    for (auto *targetMember : targetMembers) {
        auto kind = targetMember->IsMethodDefinition() ? ClassRelationKind::METHOD : ClassRelationKind::PROPERTY;
        bool isMatch = false;
        for (auto *currentMember : currentMembers) {
            if (IsMethodMatch(currentMember, targetMember)) {
                isMatch = true;
                std::string fileNamePath(currentMember->Start().ToLocation().Program()->SourceFilePath());
                matchedContainer.emplace_back(fileNamePath, currentMember->Start().index, kind);
                break;
            }
        }
        if (!isMatch) {
            std::string fileNamePath(targetMember->Start().ToLocation().Program()->SourceFilePath());
            unmatchedContainer.emplace_back(fileNamePath, targetMember->Start().index, kind);
        }
    }
}

void CompareMembersForImplementation(const std::vector<ir::AstNode *> &currentMembers,
                                     const std::vector<ir::AstNode *> &interfaceMembers, ClassHierarchyItemInfo &info,
                                     const std::string &fileName)
{
    CompareMembersCommon(currentMembers, interfaceMembers, info.implemented, info.implementing, fileName);
}

void CompareMembersForOverride(const std::vector<ir::AstNode *> &currentMembers,
                               const std::vector<ir::AstNode *> &targetMembers, ClassHierarchyItemInfo &info,
                               const std::string &fileName)
{
    CompareMembersCommon(currentMembers, targetMembers, info.overridden, info.overriding, fileName);
}

struct ProcessItemsParams {
    const std::vector<ir::AstNode *> &currentMembers;
    std::vector<ClassHierarchyItemInfo> &result;
    std::vector<ir::AstNode *> (*getListFunc)(std::vector<es2panda_Context *> *, ir::AstNode *);
    ClassRelationKind kind;
    bool swapCompareArgs;
    void (*compareFunc)(const std::vector<ir::AstNode *> &, const std::vector<ir::AstNode *> &,
                        ClassHierarchyItemInfo &, const std::string &);
};

void ProcessItems(std::vector<es2panda_Context *> *contextList, ir::AstNode *node, const std::string &fileName,
                  const ProcessItemsParams &params)
{
    auto itemList = params.getListFunc(contextList, node);
    for (auto *item : itemList) {
        std::string name = GetIdentifierName(item);
        ClassHierarchyItemInfo info(name, params.kind, item->Start().index);
        auto itemMembers = GetMembers(contextList, item);
        if (params.swapCompareArgs) {
            params.compareFunc(itemMembers, params.currentMembers, info, fileName);
        } else {
            params.compareFunc(params.currentMembers, itemMembers, info, fileName);
        }
        params.result.emplace_back(info);
    }
}

std::vector<ClassHierarchyItemInfo> GetClassHierarchiesImpl(std::vector<es2panda_Context *> *contextList,
                                                            [[maybe_unused]] const std::string &fileName, size_t pos)
{
    std::vector<ClassHierarchyItemInfo> result;
    if (contextList->empty()) {
        return result;
    }
    auto classNode = GetTargetDeclarationNodeByPosition(contextList->at(0), pos);
    if (classNode == nullptr) {
        return result;
    }
    std::vector<ir::AstNode *> currentMembers = GetMembers(contextList, classNode);
    if (classNode->IsClassDeclaration()) {
        ProcessItems(contextList, classNode, fileName,
                     ProcessItemsParams {currentMembers, result, GetClassSuperClasses, ClassRelationKind::CLASS, false,
                                         CompareMembersForOverride});
        ProcessItems(contextList, classNode, fileName,
                     ProcessItemsParams {currentMembers, result, GetClassImplementedInterfaces,
                                         ClassRelationKind::INTERFACE, false, CompareMembersForImplementation});
        ProcessItems(contextList, classNode, fileName,
                     ProcessItemsParams {currentMembers, result, GetClassSubClasses, ClassRelationKind::CLASS, true,
                                         CompareMembersForOverride});
    } else if (classNode->IsTSInterfaceDeclaration()) {
        ProcessItems(contextList, classNode, fileName,
                     ProcessItemsParams {currentMembers, result, GetInterfaceSuperInterfaces,
                                         ClassRelationKind::INTERFACE, false, CompareMembersForOverride});
        ProcessItems(contextList, classNode, fileName,
                     ProcessItemsParams {currentMembers, result, GetInterfaceSubInterfaces,
                                         ClassRelationKind::INTERFACE, true, CompareMembersForOverride});
        ProcessItems(contextList, classNode, fileName,
                     ProcessItemsParams {currentMembers, result, GetInterfaceImplementingClasses,
                                         ClassRelationKind::CLASS, false, CompareMembersForImplementation});
    }
    return result;
}

}  // namespace ark::es2panda::lsp