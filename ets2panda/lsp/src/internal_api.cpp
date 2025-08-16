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

#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include "api.h"
#include "internal_api.h"
#include "checker/types/type.h"
#include "code_fixes/code_fix_types.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "lexer/token/sourceLocation.h"
#include "macros.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "utils/arena_containers.h"
#include "formatting/formatting.h"
#include "code_fix_provider.h"
#include "get_class_property_info.h"
#include "generated/code_fix_register.h"

namespace ark::es2panda::lsp {

Initializer::Initializer()
{
    impl_ = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    std::string buildDir;
#ifdef BUILD_FOLDER
    buildDir = std::string(BUILD_FOLDER) + util::PATH_DELIMITER + "bin" + util::PATH_DELIMITER;
#endif
    const char *path = std::getenv("BUILD_FOLDER");
    if (path != nullptr) {
        buildDir = std::string(path);
    }
    std::array<const char *, 1> argv = {buildDir.c_str()};
    cfg_ = impl_->CreateConfig(argv.size(), argv.data());
    allocator_ = new ark::ArenaAllocator(ark::SpaceType::SPACE_TYPE_COMPILER);
}

Initializer::~Initializer()
{
    delete allocator_;
    impl_->DestroyConfig(cfg_);
}

ir::AstNode *GetTouchingToken(es2panda_Context *context, size_t pos, bool flagFindFirstMatch)
{
    if (context == nullptr) {
        return nullptr;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    auto ast = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    auto checkFunc = [&pos](ir::AstNode *node) { return pos >= node->Start().index && pos < node->End().index; };
    auto found = ast->FindChild(checkFunc);
    while (found != nullptr && !flagFindFirstMatch) {
        auto *nestedFound = found->FindChild(checkFunc);
        if (nestedFound == nullptr) {
            break;
        }
        found = nestedFound;
    }
    return found;
}

Position TransSourcePositionToPosition(lexer::SourcePosition sourcePos)
{
    return Position(sourcePos.line, sourcePos.index);
}

std::string FormatStringFromArgs(const std::string &textStr,
                                 const std::vector<std::string> &args = std::vector<std::string>())
{
    std::stringstream ss;
    size_t pos = 0;
    size_t start = 0;
    while ((pos = textStr.find_first_of('{', start)) != std::string::npos) {
        ss << textStr.substr(start, pos - start);
        size_t end = textStr.find('}', pos);
        if (end == std::string::npos) {
            ss << textStr.substr(pos);
            break;
        }
        std::string placeholder = textStr.substr(pos, end - pos + 1);
        std::string indexStr = placeholder.substr(1, placeholder.length() - 2);
        try {
            int index = std::stoi(indexStr);
            if (index >= 0 && index < static_cast<int>(args.size())) {
                ss << args.at(index);
            } else {
                ss << placeholder;
            }
        } catch (const std::out_of_range &e) {
            ss << placeholder;
        } catch (const std::invalid_argument &e) {
            ss << placeholder;
        }
        start = end + 1;
    }
    ss << textStr.substr(start);

    return ss.str();
}

FileDiagnostic CreateFileDiagnostic(es2panda_AstNode *node, Range span, Diagnostic diagnostic,
                                    const std::vector<std::string> &args = std::vector<std::string>())
{
    if (!args.empty()) {
        std::string newMessageStr = FormatStringFromArgs(diagnostic.message_, args);
        diagnostic.message_ = newMessageStr;
    }
    FileDiagnostic fileDiagnostic(node, diagnostic, span.start, span.end);
    return fileDiagnostic;
}

Range GetErrorRangeForNode(ir::AstNode *node, es2panda_Context *context)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    // The line and col should start from 1 istead of 0
    auto index = lexer::LineIndex(ctx->parserProgram->SourceCode());
    auto sourceStartLocation = index.GetLocation(node->Range().start);
    auto sourceEndLocation = index.GetLocation(node->Range().end);
    Position posStart(sourceStartLocation.line, sourceStartLocation.col);
    Position posEnd(sourceEndLocation.line, sourceEndLocation.col);
    return Range(posStart, posEnd);
}

FileDiagnostic CreateDiagnosticForNode(es2panda_AstNode *node, Diagnostic diagnostic, es2panda_Context *context,
                                       const std::vector<std::string> &args)
{
    auto span = diagnostic.range_;
    // Only genereate range when original range is invalid
    if (span.start.character_ <= 0 || span.end.character_ <= 0 || span.start.line_ <= 0 || span.end.line_ <= 0) {
        span = GetErrorRangeForNode(reinterpret_cast<ir::AstNode *>(node), context);
    }
    auto res = CreateFileDiagnostic(node, span, std::move(diagnostic), args);
    return res;
}

References GetFileReferencesImpl(es2panda_Context *referenceFileContext, char const *searchFileName,
                                 bool isPackageModule)
{
    References result;
    auto ctx = reinterpret_cast<public_lib::Context *>(referenceFileContext);
    auto statements = ctx->parserProgram->Ast()->Statements();
    for (auto statement : statements) {
        if (!statement->IsETSImportDeclaration()) {
            continue;
        }
        auto import = statement->AsETSImportDeclaration();
        std::string importFileName {import->ResolvedSource()};
        if (!import->Source()->IsStringLiteral()) {
            continue;
        }
        auto start = import->Source()->Start().index;
        auto end = import->Source()->End().index;
        auto pos = std::string(searchFileName).rfind(util::PATH_DELIMITER);
        auto fileDirectory = std::string(searchFileName).substr(0, pos);
        if ((!isPackageModule && importFileName == searchFileName) ||
            (isPackageModule && importFileName == fileDirectory)) {
            auto fileName = ctx->sourceFileName;
            result.referenceInfos.emplace_back(fileName, start, end - start);
        }
    }
    return result;
}

bool IsToken(const ir::AstNode *node)
{
    /**
     * True if node is of some token node.
     * For example, this is true for an IDENTIFIER or NUMBER_LITERAL but not for BLOCK_STATEMENT or CallExpression.
     * Keywords like "if" and "of" exist as TOKEN_TYPE and cannot be recognized as AstNode, so returning nodes like
     * IfKeyword or OfKeyword is not supported.
     */
    return node->Type() == ir::AstNodeType::BIGINT_LITERAL || node->Type() == ir::AstNodeType::BOOLEAN_LITERAL ||
           node->Type() == ir::AstNodeType::CHAR_LITERAL || node->Type() == ir::AstNodeType::IDENTIFIER ||
           node->Type() == ir::AstNodeType::NULL_LITERAL || node->Type() == ir::AstNodeType::UNDEFINED_LITERAL ||
           node->Type() == ir::AstNodeType::NUMBER_LITERAL || node->Type() == ir::AstNodeType::REGEXP_LITERAL ||
           node->Type() == ir::AstNodeType::STRING_LITERAL || node->Type() == ir::AstNodeType::TS_NUMBER_KEYWORD ||
           node->Type() == ir::AstNodeType::TS_ANY_KEYWORD || node->Type() == ir::AstNodeType::TS_BOOLEAN_KEYWORD ||
           node->Type() == ir::AstNodeType::TS_VOID_KEYWORD || node->Type() == ir::AstNodeType::TS_UNDEFINED_KEYWORD ||
           node->Type() == ir::AstNodeType::TS_UNKNOWN_KEYWORD || node->Type() == ir::AstNodeType::TS_OBJECT_KEYWORD ||
           node->Type() == ir::AstNodeType::TS_BIGINT_KEYWORD || node->Type() == ir::AstNodeType::TS_NEVER_KEYWORD ||
           node->Type() == ir::AstNodeType::TS_NULL_KEYWORD || node->Type() == ir::AstNodeType::TEMPLATE_ELEMENT;
}

bool IsNonWhitespaceToken(const ir::AstNode *node)
{
    return IsToken(node);
}

bool NodeHasTokens(const ir::AstNode *node)
{
    return node->Start().index != node->End().index;
}

ir::AstNode *FindRightmostChildNodeWithTokens(const ArenaVector<ir::AstNode *> &nodes, int exclusiveStartPosition)
{
    for (int i = exclusiveStartPosition - 1; i >= 0; --i) {
        if (NodeHasTokens(nodes[i])) {
            return nodes[i];
        }
    }
    return nullptr;
}

ArenaVector<ir::AstNode *> GetChildren(const ir::AstNode *node, ArenaAllocator *allocator)
{
    ArenaVector<ir::AstNode *> children(allocator->Adapter());
    if (node->Type() == ir::AstNodeType::ETS_MODULE) {
        // ETS_MODULE is the root node, need to get the definition of global class
        auto globalClass =
            node->FindChild([](ir::AstNode *child) { return child->IsClassDeclaration(); })->AsClassDeclaration();
        node = globalClass->Definition();
    }
    node->Iterate([&children](ir::AstNode *child) { children.push_back(child); });
    return children;
}

ir::AstNode *FindRightmostToken(const ir::AstNode *node, ArenaAllocator *allocator)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (IsNonWhitespaceToken(node)) {
        return const_cast<ir::AstNode *>(node);
    }
    auto children = GetChildren(node, allocator);
    if (children.empty()) {
        return const_cast<ir::AstNode *>(node);
    }
    auto candidate = FindRightmostChildNodeWithTokens(children, children.size());
    return FindRightmostToken(candidate, allocator);
}

ir::AstNode *FindNodeBeforePosition(const ArenaVector<ir::AstNode *> &children, size_t pos)
{
    if (children.empty()) {
        return nullptr;
    }
    size_t left = 0;
    size_t right = children.size() - 1;
    size_t mid = 0;
    while (left <= right) {
        mid = left + ((right - left) >> 1U);
        if (pos < children[mid]->End().index) {
            if (mid == 0 || pos >= children[mid - 1]->End().index) {
                break;
            }
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return FindRightmostChildNodeWithTokens(children, mid);
}

ir::AstNode *FindPrecedingToken(const size_t pos, const ir::AstNode *startNode, ArenaAllocator *allocator)
{
    auto checkFunc = [&pos](ir::AstNode *node) { return node->Start().index <= pos && pos <= node->End().index; };
    auto found = startNode->FindChild(checkFunc);
    if (found != nullptr) {
        auto nestedFound = found->FindChild(checkFunc);
        while (nestedFound != nullptr) {
            // try to find the minimum node that embraces position
            found = nestedFound;
            nestedFound = found->FindChild(checkFunc);
        }

        // position is 0, found does not has any tokens
        if (!NodeHasTokens(found)) {
            return nullptr;
        }

        if (IsNonWhitespaceToken(found)) {
            return found;
        }

        // found embraces the position, but none of its children do
        // (ie: in a comment or whitespace preceding `child node`)
        auto children = GetChildren(found, allocator);
        auto candidate = FindNodeBeforePosition(children, pos);
        return FindRightmostToken(candidate, allocator);
    }

    // position is in the global scope but not 0, found will be nullptr.
    auto children = GetChildren(startNode, allocator);
    auto candidate = FindNodeBeforePosition(children, pos);
    return FindRightmostToken(candidate, allocator);
}

ir::AstNode *GetOriginalNode(ir::AstNode *astNode)
{
    while (astNode != nullptr && astNode->OriginalNode() != nullptr) {
        astNode = astNode->OriginalNode();
    }
    return astNode;
}

checker::VerifiedType GetTypeOfSymbolAtLocation(checker::ETSChecker *checker, ir::AstNode *astNode)
{
    ES2PANDA_ASSERT(astNode);
    auto originalNode = GetOriginalNode(astNode);
    return originalNode->Check(checker);
}

std::string ReplaceQuotation(ark::es2panda::util::StringView strView)
{
    std::string str = std::string {strView};
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\''), str.end());
    return str;
}

std::string GetCurrentTokenValueImpl(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto program = ctx->parserProgram;
    auto ast = program->Ast();
    ir::AstNode *node = FindPrecedingToken(position, ast, ctx->allocator);
    return node != nullptr ? ReplaceQuotation(program->SourceCode().Substr(node->Start().index, position)) : "";
}

ir::AstNode *FindLeftToken(const size_t pos, const ArenaVector<ir::AstNode *> &nodes)
{
    int left = 0;
    int right = nodes.size() - 1;
    ir::AstNode *result = nullptr;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (nodes[mid]->End().index <= pos) {
            result = nodes[mid];
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

ir::AstNode *FindRightToken(const size_t pos, const ArenaVector<ir::AstNode *> &nodes)
{
    int left = 0;
    int right = nodes.size() - 1;
    ir::AstNode *result = nullptr;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (nodes[mid]->Start().index > pos) {
            result = nodes[mid];
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return result;
}

void GetRangeOfCommentFromContext(std::string const &sourceCode, size_t leftPos, size_t rightPos, size_t pos,
                                  CommentRange *result)
{
    constexpr size_t BLOCK_COMMENT_START_LENGTH = 2;
    std::vector<CommentRange> commentRanges;
    size_t startIndex = 0;
    while (startIndex < sourceCode.size()) {
        size_t blockCommentStart = sourceCode.find("/*", startIndex);
        size_t lineCommentStart = sourceCode.find("//", startIndex);
        if (blockCommentStart == std::string::npos && lineCommentStart == std::string::npos) {
            break;
        }
        if (blockCommentStart < lineCommentStart || lineCommentStart == std::string::npos) {
            if (blockCommentStart > rightPos) {
                break;
            }
            size_t blockCommentEnd = sourceCode.find("*/", blockCommentStart + BLOCK_COMMENT_START_LENGTH);
            if (blockCommentEnd == std::string::npos) {
                break;
            }
            commentRanges.emplace_back(
                CommentRange(blockCommentStart, blockCommentEnd + BLOCK_COMMENT_START_LENGTH, CommentKind::MULTI_LINE));
            startIndex = blockCommentEnd + BLOCK_COMMENT_START_LENGTH;
            continue;
        }
        if (lineCommentStart > rightPos) {
            break;
        }
        size_t lineCommentEnd = sourceCode.find('\n', lineCommentStart);
        if (lineCommentEnd == std::string::npos) {
            lineCommentEnd = sourceCode.size();
        }
        commentRanges.emplace_back(CommentRange(lineCommentStart, lineCommentEnd, CommentKind::SINGLE_LINE));
        startIndex = lineCommentEnd;
    }
    for (const auto &range : commentRanges) {
        if (range.pos_ <= pos && range.end_ >= pos && range.pos_ >= leftPos && range.end_ <= rightPos) {
            result->pos_ = range.pos_;
            result->kind_ = range.kind_;
            result->end_ = range.end_;
            break;
        }
    }
}

void GetRangeOfEnclosingComment(es2panda_Context *context, size_t pos, CommentRange *result)
{
    auto touchingNode = GetTouchingToken(context, pos, false);
    if (touchingNode != nullptr && IsToken(touchingNode)) {
        return;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto ast = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    ir::AstNode *parent = touchingNode != nullptr ? touchingNode : ast;
    auto children = GetChildren(parent, ctx->allocator);
    std::sort(children.begin(), children.end(), [](ir::AstNode *a, ir::AstNode *b) {
        if (a->Start().index < b->Start().index) {
            return true;
        }
        if (a->Start().index == b->Start().index) {
            return a->End().index < b->End().index;
        }
        return false;
    });
    ir::AstNode *leftToken = FindLeftToken(pos, children);
    ir::AstNode *rightToken = FindRightToken(pos, children);
    size_t leftPos = leftToken != nullptr ? leftToken->End().index : parent->Start().index;
    size_t rightPos = rightToken != nullptr ? rightToken->Start().index : parent->End().index;
    std::string sourceCode(ctx->parserProgram->SourceCode());
    GetRangeOfCommentFromContext(sourceCode, leftPos, rightPos, pos, result);
}

void RemoveFromFiles(std::vector<std::string> &files, const std::vector<std::string> &autoGenerateFolders)
{
    auto rm = [&autoGenerateFolders](const std::string &file) {
        return std::any_of(autoGenerateFolders.begin(), autoGenerateFolders.end(),
                           [&file](const std::string &folder) { return file.find(folder) != std::string::npos; });
    };

    files.erase(std::remove_if(files.begin(), files.end(), rm), files.end());
    if (files.empty()) {
        return;
    }
}

void SaveNode(ir::AstNode *node, public_lib::Context *ctx, ReferenceLocationList *result)
{
    auto uri = ctx->sourceFileName;
    auto start = node->Range().start.index;
    auto end = node->Range().end.index;
    auto accessKind = node->IsReadonly() ? AccessKind::READ : AccessKind::WRITE;
    auto isDefinition = node->IsClassDefinition() && node->IsMethodDefinition();
    auto isImport = node->IsImportDeclaration();
    auto ref = ReferenceLocation {uri, start, end, isDefinition, accessKind, isImport};
    auto it =
        std::find_if(result->referenceLocation.begin(), result->referenceLocation.end(),
                     [&](const ReferenceLocation &loc) { return (loc.uri == ref.uri) && (loc.start == ref.start); });
    if (it == result->referenceLocation.end()) {
        result->referenceLocation.push_back(ref);
    }
}

ir::AstNode *GetIdentifier(ir::AstNode *node)
{
    if (!node->IsIdentifier()) {
        return node->FindChild([](ir::AstNode *node1) { return node1->IsIdentifier(); });
    }
    return node;
}

std::string GetIdentifierName(ir::AstNode *node)
{
    auto id = GetIdentifier(node);
    if (id == nullptr) {
        return "";
    }
    return std::string {id->AsIdentifier()->Name()};
}

ir::AstNode *GetOwner(ir::AstNode *node)
{
    auto id = GetIdentifier(node);
    if (id == nullptr) {
        return nullptr;
    }
    auto var = id->AsIdentifier()->Variable();
    if (var == nullptr) {
        return nullptr;
    }
    auto decl = id->AsIdentifier()->Variable()->Declaration();
    if (decl == nullptr) {
        return nullptr;
    }
    return decl->Node();
}

std::string GetOwnerId(ir::AstNode *node)
{
    auto owner = GetOwner(node);
    if (owner == nullptr) {
        return "";
    }
    return owner->DumpJSON();
}

// convert from es2panda error type to LSP severity
DiagnosticSeverity GetSeverity(util::DiagnosticType errorType)
{
    ES2PANDA_ASSERT(errorType != util::DiagnosticType::INVALID);
    if (errorType == util::DiagnosticType::WARNING || errorType == util::DiagnosticType::PLUGIN_WARNING) {
        return DiagnosticSeverity::Warning;
    }
    if (errorType == util::DiagnosticType::SYNTAX || errorType == util::DiagnosticType::SEMANTIC ||
        errorType == util::DiagnosticType::FATAL || errorType == util::DiagnosticType::ARKTS_CONFIG_ERROR ||
        errorType == util::DiagnosticType::PLUGIN_ERROR) {
        return DiagnosticSeverity::Error;
    }
    throw std::runtime_error("Unknown error type!");
}

// Temp design only support UI Plugin Diag.
int CreateCodeForDiagnostic(const util::DiagnosticBase *error)
{
    const int uiCode = 4000;
    if (error->Type() == util::DiagnosticType::PLUGIN_ERROR || error->Type() == util::DiagnosticType::PLUGIN_WARNING) {
        return uiCode;
    }
    auto code = static_cast<const util::Diagnostic *>(error)->GetId();
    return static_cast<int>(error->Type()) * codefixes::DiagnosticCode::DIAGNOSTIC_CODE_MULTIPLIER + code;
}

Diagnostic CreateDiagnosticForError(es2panda_Context *context, const util::DiagnosticBase &error)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto index = lexer::LineIndex(ctx->parserProgram->SourceCode());
    auto offset = index.GetOffset(lexer::SourceLocation(error.Line(), error.Offset(), ctx->parserProgram));
    auto touchingToken = GetTouchingToken(context, offset, false);
    lexer::SourceRange sourceRange;
    lexer::SourceLocation sourceStartLocation;
    lexer::SourceLocation sourceEndLocation;
    std::string source;
    if (touchingToken == nullptr) {
        sourceStartLocation = lexer::SourceLocation(error.Line(), error.Offset(), ctx->parserProgram);
        sourceEndLocation = lexer::SourceLocation(error.Line(), error.Offset(), ctx->parserProgram);
        source = "";
    } else {
        sourceRange = touchingToken->Range();
        sourceStartLocation = index.GetLocation(sourceRange.start);
        sourceEndLocation = index.GetLocation(sourceRange.end);
        source = touchingToken->DumpEtsSrc();
    }
    auto range = Range(Position(sourceStartLocation.line, sourceStartLocation.col),
                       Position(sourceEndLocation.line, sourceEndLocation.col));
    auto severity = GetSeverity(error.Type());
    auto code = CreateCodeForDiagnostic(&error);
    std::string message = error.Message();
    auto codeDescription = CodeDescription("test code description");
    auto tags = std::vector<DiagnosticTag>();
    auto relatedInformation = std::vector<DiagnosticRelatedInformation>();
    return Diagnostic(range, tags, relatedInformation, severity, code, message, codeDescription, source);
}

Diagnostic CreateDiagnosticWithoutFile(const util::DiagnosticBase &error)
{
    auto range = Range(Position(), Position());
    auto severity = GetSeverity(error.Type());
    auto code = 1;
    std::string message = error.Message();
    auto codeDescription = CodeDescription("test code description");
    auto tags = std::vector<DiagnosticTag>();
    auto relatedInformation = std::vector<DiagnosticRelatedInformation>();
    return Diagnostic(range, tags, relatedInformation, severity, code, message, codeDescription, "");
}

void GetGlobalDiagnostics(es2panda_Context *context, DiagnosticReferences &compilerOptionsDiagnostics)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::FATAL);
    for (const auto &diagnostic : diagnostics) {
        if (diagnostic->File().empty()) {
            compilerOptionsDiagnostics.diagnostic.push_back(CreateDiagnosticWithoutFile(*diagnostic));
        } else {
            compilerOptionsDiagnostics.diagnostic.push_back(CreateDiagnosticForError(context, *diagnostic));
        }
    }
}

void GetOptionDiagnostics(es2panda_Context *context, DiagnosticReferences &compilerOptionsDiagnostics)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::ARKTS_CONFIG_ERROR);
    for (const auto &diagnostic : diagnostics) {
        compilerOptionsDiagnostics.diagnostic.push_back(CreateDiagnosticWithoutFile(*diagnostic));
    }
}

size_t GetTokenPosOfNode(const ir::AstNode *astNode)
{
    ES2PANDA_ASSERT(astNode);

    return astNode->Start().index;
}

std::pair<ir::AstNode *, util::StringView> GetDefinitionAtPositionImpl(es2panda_Context *context, size_t pos)
{
    std::pair<ir::AstNode *, util::StringView> res;
    auto node = GetTouchingToken(context, pos, false);
    if (node == nullptr) {
        return res;
    }
    if (node->IsCallExpression()) {
        node = node->AsCallExpression()->Callee()->AsIdentifier();
    }
    if (!node->IsIdentifier()) {
        return res;
    }
    res = {compiler::DeclarationFromIdentifier(node->AsIdentifier()), node->AsIdentifier()->Name()};
    return res;
}

std::string GetImportFilePath(es2panda_Context *context, size_t pos)
{
    std::string res;
    auto node = GetTouchingToken(context, pos, false);
    if (node == nullptr) {
        return res;
    }
    auto parent = node->Parent();
    if (parent != nullptr && parent->IsETSImportDeclaration() && parent->AsETSImportDeclaration()->Source() == node) {
        res = std::string(parent->AsETSImportDeclaration()->ImportMetadata().resolvedSource);
    }
    return res;
}

ArenaVector<ir::AstNode *> RemoveRefDuplicates(const ArenaVector<ir::AstNode *> &nodes, ArenaAllocator *allocator)
{
    auto hashFunc = [](const ir::AstNode *node) {
        return std::hash<int>()(node->Start().index) ^ std::hash<int>()(node->End().index);
    };
    auto equalFunc = [](const ir::AstNode *lhs, const ir::AstNode *rhs) {
        return lhs->Start().index == rhs->Start().index && lhs->End().index == rhs->End().index;
    };

    ArenaUnorderedSet<ir::AstNode *, decltype(hashFunc), decltype(equalFunc)> uniqueNodes(
        nodes.size(), hashFunc, equalFunc, allocator->Adapter());
    for (auto node : nodes) {
        uniqueNodes.insert(node);
    }

    return ArenaVector<ir::AstNode *>(uniqueNodes.begin(), uniqueNodes.end(), allocator->Adapter());
}

void FindAllChildHelper(ir::AstNode *ast, const ir::NodePredicate &cb, ArenaVector<ir::AstNode *> &results)
{
    if (cb(ast)) {
        results.push_back(ast);
    }

    ast->Iterate([&results, cb](ir::AstNode *child) { FindAllChildHelper(child, cb, results); });
}

void FindAllChild(const ir::AstNode *ast, const ir::NodePredicate &cb, ArenaVector<ir::AstNode *> &results)
{
    ast->Iterate([&results, cb](ir::AstNode *child) { FindAllChildHelper(child, cb, results); });
}

ir::AstNode *FindAncestor(ir::AstNode *node, const ir::NodePredicate &cb)
{
    while (node != nullptr) {
        if (cb(node)) {
            return node;
        }
        node = node->Parent();
    }
    return node;
}

ArenaVector<ir::AstNode *> FindReferencesByName(ir::AstNode *ast, ir::AstNode *decl, ir::AstNode *node,
                                                ArenaAllocator *allocator)
{
    ASSERT(node->IsIdentifier());
    auto name = node->AsIdentifier()->Name();
    auto checkFunc = [&name, &decl](ir::AstNode *child) {
        return child->IsIdentifier() && compiler::DeclarationFromIdentifier(child->AsIdentifier()) == decl &&
               child->AsIdentifier()->Name() == name;
    };
    auto references = ArenaVector<ir::AstNode *>(allocator->Adapter());
    FindAllChild(ast, checkFunc, references);

    auto uniqueReferences = RemoveRefDuplicates(references, allocator);
    std::sort(uniqueReferences.begin(), uniqueReferences.end(), [](const ir::AstNode *a, const ir::AstNode *b) {
        return (a->Start().index != b->Start().index) ? a->Start().index < b->Start().index
                                                      : a->End().index < b->End().index;
    });
    return uniqueReferences;
}

HighlightSpanKind GetHightlightSpanKind(ir::AstNode *identifierDeclaration, ir::Identifier *node)
{
    if (identifierDeclaration->Start().index == node->Start().index &&
        identifierDeclaration->End().index == node->End().index) {
        return HighlightSpanKind::WRITTEN_REFERENCE;
    }
    return HighlightSpanKind::REFERENCE;
}

DocumentHighlights GetSemanticDocumentHighlights(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto ast = ctx->parserProgram->Ast();
    std::string fileName(ctx->sourceFile->filePath);
    auto touchingToken = GetTouchingToken(context, position, false);
    if (!touchingToken->IsIdentifier()) {
        return DocumentHighlights(fileName, {});
    }
    auto decl = compiler::DeclarationFromIdentifier(touchingToken->AsIdentifier());
    if (decl == nullptr) {
        return DocumentHighlights(fileName, {});
    }
    auto checkFunc = [&touchingToken](ir::AstNode *child) {
        return child->IsIdentifier() && child->AsIdentifier()->Name() == touchingToken->AsIdentifier()->Name();
    };
    // Find the identifier's declaration. We consider the first found to be the identifier's declaration.
    auto identifierDeclaration = decl->FindChild(checkFunc);
    if (identifierDeclaration == nullptr) {
        // If the identifier is not found in the declaration, we try to find it in the AST.
        // This is needed for cases like `import {Foo as foo} from './a';` where
        // `foo` is a alias for the imported `Foo` identifier.
        identifierDeclaration = ast->FindChild(checkFunc);
    }
    if (identifierDeclaration == nullptr) {
        return DocumentHighlights(fileName, {});
    }

    auto references = FindReferencesByName(ast, decl, touchingToken, ctx->allocator);

    auto highlightSpans = std::vector<HighlightSpan>();
    for (const auto &reference : references) {
        auto start = reference->Start().index;
        auto length = reference->AsIdentifier()->Name().Length();
        TextSpan textSpan = {start, length};
        TextSpan contextSpan = {0, 0};
        auto kind = GetHightlightSpanKind(identifierDeclaration, reference->AsIdentifier());
        highlightSpans.emplace_back(fileName, false, textSpan, contextSpan, kind);
    }
    return DocumentHighlights(fileName, highlightSpans);
}

DocumentHighlights GetDocumentHighlightsImpl(es2panda_Context *context, size_t position)
{
    return GetSemanticDocumentHighlights(context, position);
}

std::vector<InstallPackageActionInfo> CreateInstallPackageActionInfos(std::vector<InstallPackageAction> &commands)
{
    std::vector<InstallPackageActionInfo> infos;
    for (const auto &command : commands) {
        InstallPackageActionInfo info {command.type, command.file, command.packageName};
        infos.push_back(info);
    }

    return infos;
}

CodeFixActionInfo CreateCodeFixActionInfo(CodeFixAction &codeFixAction)
{
    auto infos = CreateInstallPackageActionInfos(codeFixAction.commands);

    CodeActionInfo codeActionInfo {codeFixAction.description, codeFixAction.changes, infos};

    return CodeFixActionInfo {codeActionInfo, codeFixAction.fixName, codeFixAction.fixId,
                              codeFixAction.fixAllDescription};
}

CombinedCodeActionsInfo CreateCombinedCodeActionsInfo(CombinedCodeActions &combinedCodeActions)
{
    auto infos = CreateInstallPackageActionInfos(combinedCodeActions.commands);

    return CombinedCodeActionsInfo {combinedCodeActions.changes, infos};
}

std::vector<CodeFixActionInfo> GetCodeFixesAtPositionImpl(es2panda_Context *context, size_t startPosition,
                                                          size_t endPosition, std::vector<int> &errorCodes,
                                                          CodeFixOptions &codeFixOptions)
{
    TextSpan textspan = TextSpan(startPosition, endPosition);
    std::vector<CodeFixActionInfo> actions;
    auto formatContext = GetFormatContext(codeFixOptions.options);

    for (auto errorCode : errorCodes) {
        if (codeFixOptions.token.IsCancellationRequested()) {
            return actions;
        }

        TextChangesContext textChangesContext {LanguageServiceHost(), formatContext, codeFixOptions.preferences};
        CodeFixContextBase codeFixContextBase {textChangesContext, context, codeFixOptions.token};
        CodeFixContext codeFixContent {codeFixContextBase, errorCode, textspan};

        auto fixes = CodeFixProvider::Instance().GetFixes(codeFixContent);
        for (auto fix : fixes) {
            auto codeFixes = CreateCodeFixActionInfo(fix);
            actions.push_back(codeFixes);
        }
    }

    return actions;
}

CombinedCodeActionsInfo GetCombinedCodeFixImpl(es2panda_Context *context, const std::string &fixId,
                                               CodeFixOptions &codeFixOptions)
{
    auto formatContext = GetFormatContext(codeFixOptions.options);
    TextChangesContext textChangesContext {LanguageServiceHost(), formatContext, codeFixOptions.preferences};
    CodeFixContextBase codeFixContextBase {textChangesContext, context, codeFixOptions.token};
    CodeFixAllContext codeFixAllContent {codeFixContextBase, fixId};

    auto fixes = CodeFixProvider::Instance().GetAllFixes(codeFixAllContent);

    return CreateCombinedCodeActionsInfo(fixes);
}

varbinder::Decl *FindDeclInGlobalScope(varbinder::Scope *scope, const util::StringView &name)
{
    const auto *scopeIter = scope;
    varbinder::Decl *resolved = nullptr;
    while (scopeIter != nullptr && !scopeIter->IsGlobalScope()) {
        bool isModule = scopeIter->Node() != nullptr && scopeIter->Node()->IsClassDefinition() &&
                        scopeIter->Node()->AsClassDefinition()->IsModule();
        if (isModule) {
            resolved = scopeIter->FindDecl(name);
            if (resolved != nullptr) {
                break;
            }
        }
        scopeIter = scopeIter->Parent();
    }
    if (resolved == nullptr && scopeIter != nullptr && scopeIter->IsGlobalScope()) {
        resolved = scopeIter->FindDecl(name);
    }
    return resolved;
}

varbinder::Decl *FindDeclInFunctionScope(varbinder::Scope *scope, const util::StringView &name)
{
    const auto *scopeIter = scope;
    while (scopeIter != nullptr && !scopeIter->IsGlobalScope()) {
        if (!scopeIter->IsClassScope()) {
            if (auto *const resolved = scopeIter->FindDecl(name); resolved != nullptr) {
                return resolved;
            }
        }
        scopeIter = scopeIter->Parent();
    }

    return nullptr;
}

varbinder::Decl *FindDeclInScopeWithFallback(varbinder::Scope *scope, const util::StringView &name)
{
    if (auto *decl = FindDeclInFunctionScope(scope, name)) {
        return decl;
    }
    return FindDeclInGlobalScope(scope, name);
}

}  // namespace ark::es2panda::lsp
