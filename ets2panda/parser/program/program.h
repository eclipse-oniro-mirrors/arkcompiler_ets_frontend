/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_PARSER_INCLUDE_PROGRAM_H
#define ES2PANDA_PARSER_INCLUDE_PROGRAM_H

#include "util/es2pandaMacros.h"
#include "mem/pool_manager.h"
#include "os/filesystem.h"
#include "util/ustring.h"
#include "util/path.h"
#include "util/importPathManager.h"
#include "varbinder/varbinder.h"
#include <lexer/token/sourceLocation.h>
#include "util/enumbitops.h"

#include <set>
#include <ir/statements/blockStatement.h>

namespace ark::es2panda::ir {
class BlockStatement;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
class VarBinder;
class FunctionScope;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::compiler {
class CFG;
}  // namespace ark::es2panda::compiler

namespace ark::es2panda::checker {
class Checker;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::parser {
enum class ScriptKind { SCRIPT, MODULE, STDLIB, GENEXTERNAL };

#ifndef NDEBUG
constexpr uint32_t POISON_VALUE {0x12346789};
#endif

using ENUMBITOPS_OPERATORS;

enum class ProgramFlags : uint32_t {
    NONE = 0U,
    AST_CHECKED = 1U << 0U,
    AST_CHECK_PROCESSED = 1U << 1U,
    AST_ENUM_LOWERED = 1U << 2U,
    AST_BOXED_TYPE_LOWERED = 1U << 3U,
    AST_CONSTANT_EXPRESSION_LOWERED = 1U << 5U,
    AST_STRING_CONSTANT_LOWERED = 1U << 6U,
    AST_IDENTIFIER_ANALYZED = 1U << 7U,
    AST_HAS_SCOPES_INITIALIZED = 1U << 8U,
    AST_HAS_OPTIONAL_PARAMETER_ANNOTATION = 1U << 9U,
};

class Program {
public:
    using ExternalSource = ArenaUnorderedMap<util::StringView, ArenaVector<Program *>>;
    using DirectExternalSource = ArenaUnorderedMap<util::StringView, ArenaVector<Program *>>;

    using ETSNolintsCollectionMap = ArenaUnorderedMap<const ir::AstNode *, ArenaSet<ETSWarnings>>;

    template <typename T>
    static Program NewProgram(ArenaAllocator *allocator, varbinder::VarBinder *varBinder = nullptr)
    {
        if (varBinder == nullptr) {
            auto *vb = allocator->New<T>(allocator);
            return Program(allocator, vb);
        }
        return Program(allocator, varBinder);
    }

    Program(ArenaAllocator *allocator, varbinder::VarBinder *varbinder);

    ~Program();

    void SetKind(ScriptKind kind)
    {
        kind_ = kind;
    }

    NO_COPY_SEMANTIC(Program);
    DEFAULT_MOVE_SEMANTIC(Program);

    ArenaAllocator *Allocator() const
    {
        return allocator_;
    }

    void PushVarBinder(varbinder::VarBinder *varbinder);

    const varbinder::VarBinder *VarBinder() const;

    varbinder::VarBinder *VarBinder();

    checker::Checker *Checker();
    const checker::Checker *Checker() const;

    void PushChecker(checker::Checker *checker);

    ScriptExtension Extension() const
    {
        return extension_;
    }

    ScriptKind Kind() const
    {
        return kind_;
    }

    util::StringView SourceCode() const
    {
        return sourceCode_;
    }

    const util::StringView &SourceFilePath() const
    {
        return sourceFile_.GetPath();
    }

    const util::Path &SourceFile() const
    {
        return sourceFile_;
    }

    util::StringView SourceFileFolder() const
    {
        return sourceFileFolder_;
    }

    util::StringView FileName() const
    {
        return sourceFile_.GetFileName();
    }

    util::StringView FileNameWithExtension() const
    {
        return sourceFile_.GetFileNameWithExtension();
    }

    util::StringView AbsoluteName() const
    {
        return sourceFile_.GetAbsolutePath();
    }

    util::StringView ResolvedFilePath() const
    {
        return resolvedFilePath_;
    }

    util::StringView RelativeFilePath() const
    {
        // for js source files, just return file name.
        return relativeFilePath_.Empty() ? FileNameWithExtension() : relativeFilePath_;
    }

    void SetRelativeFilePath(const util::StringView &relPath)
    {
        relativeFilePath_ = relPath;
    }

    ir::BlockStatement *Ast()
    {
        return ast_;
    }

    const ir::BlockStatement *Ast() const
    {
        return ast_;
    }

    void SetAst(ir::BlockStatement *ast)
    {
        ast_ = ast;
        MaybeTransformToDeclarationModule();
    }

    ir::ClassDefinition *GlobalClass();

    const ir::ClassDefinition *GlobalClass() const;

    void SetGlobalClass(ir::ClassDefinition *globalClass);

    ExternalSource &ExternalSources()
    {
        return externalSources_;
    }

    const ExternalSource &ExternalSources() const
    {
        return externalSources_;
    }

    DirectExternalSource &DirectExternalSources()
    {
        return directExternalSources_;
    }

    const DirectExternalSource &DirectExternalSources() const
    {
        return directExternalSources_;
    }

    const lexer::SourcePosition &PackageStart() const
    {
        return packageStartPosition_;
    }

    void SetPackageStart(const lexer::SourcePosition &start)
    {
        packageStartPosition_ = start;
    }

    void SetSource(const util::StringView &sourceCode, const util::StringView &sourceFilePath,
                   const util::StringView &sourceFileFolder)
    {
        sourceCode_ = sourceCode;
        sourceFile_ = util::Path(sourceFilePath, Allocator());
        sourceFileFolder_ = sourceFileFolder;
    }

    void SetSource(const ark::es2panda::SourceFile &sourceFile)
    {
        sourceCode_ = util::UString(sourceFile.source, Allocator()).View();
        sourceFile_ = util::Path(sourceFile.filePath, Allocator());
        sourceFileFolder_ = util::UString(sourceFile.fileFolder, Allocator()).View();
        resolvedFilePath_ = util::UString(sourceFile.resolvedPath, Allocator()).View();
        moduleInfo_.isDeclForDynamicStaticInterop = sourceFile.isDeclForDynamicStaticInterop;
    }

    void SetPackageInfo(const util::StringView &name, util::ModuleKind kind);

    const auto &ModuleInfo() const
    {
        return moduleInfo_;
    }

    util::StringView ModuleName() const
    {
        return moduleInfo_.moduleName;
    }

    util::StringView ModulePrefix() const
    {
        return moduleInfo_.modulePrefix;
    }

    bool IsSeparateModule() const
    {
        return moduleInfo_.kind == util::ModuleKind::MODULE;
    }

    bool IsDeclarationModule() const
    {
        return moduleInfo_.kind == util::ModuleKind::DECLARATION;
    }

    bool IsPackage() const
    {
        return moduleInfo_.kind == util::ModuleKind::PACKAGE;
    }

    bool IsDeclForDynamicStaticInterop() const
    {
        return moduleInfo_.isDeclForDynamicStaticInterop;
    }

    void SetFlag(ProgramFlags flag);
    bool GetFlag(ProgramFlags flag) const;
    void SetASTChecked();
    void RemoveAstChecked();
    bool IsASTChecked();

    void MarkASTAsLowered()
    {
        isASTlowered_ = true;
    }

    bool IsASTLowered() const
    {
        return isASTlowered_;
    }

    bool IsStdLib() const
    {
        // NOTE (hurton): temporary solution, needs rework when std sources are renamed
        return (ModuleName().Mutf8().rfind("std.", 0) == 0) || (ModuleName().Mutf8().rfind("escompat", 0) == 0) ||
               (FileName().Is("etsstdlib"));
    }

    bool IsGenAbcForExternal() const;

    void SetGenAbcForExternalSources(bool genAbc = true)
    {
        genAbcForExternalSource_ = genAbc;
    }

    varbinder::ClassScope *GlobalClassScope();
    const varbinder::ClassScope *GlobalClassScope() const;

    varbinder::GlobalScope *GlobalScope();
    const varbinder::GlobalScope *GlobalScope() const;

    std::string Dump() const;

    void DumpSilent() const;

    void AddNodeToETSNolintCollection(const ir::AstNode *node, const std::set<ETSWarnings> &warningsCollection);
    bool NodeContainsETSNolint(const ir::AstNode *node, ETSWarnings warning);

    bool MergeExternalSource(const ExternalSource *externalSource);

    // The name "IsDied", because correct value of canary is a necessary condition for the life of "Program", but
    // not sufficient
    bool IsDied() const
    {
        // You can't add one method to ignore list of es2panda_lib generation,
        // so in release mode method is exist, return "false" and is not used anywhere.
#ifndef NDEBUG
        return poisonValue_ != POISON_VALUE;
#else
        return false;
#endif
    }

    compiler::CFG *GetCFG();
    const compiler::CFG *GetCFG() const;

    [[nodiscard]] const ArenaVector<varbinder::FunctionScope *> &Functions() const noexcept
    {
        return functionScopes_;
    }

    [[nodiscard]] ArenaVector<varbinder::FunctionScope *> &Functions() noexcept
    {
        return functionScopes_;
    }

    void AddToFunctionScopes(varbinder::FunctionScope *funcScope)
    {
        functionScopes_.push_back(funcScope);
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> &GetFileDependencies()
    {
        return fileDependencies_;
    }

    void AddFileDependencies(const std::string &file, const std::string &depFile)
    {
        if (fileDependencies_.count(file) == 0U) {
            fileDependencies_[file] = std::unordered_set<std::string>();
        }
        fileDependencies_[file].insert(depFile);
    }

private:
    void MaybeTransformToDeclarationModule();

private:
    ArenaAllocator *allocator_ {};
    ir::BlockStatement *ast_ {};
    util::StringView sourceCode_ {};
    util::Path sourceFile_ {};
    util::StringView sourceFileFolder_ {};
    util::StringView resolvedFilePath_ {};
    util::StringView relativeFilePath_ {};
    ExternalSource externalSources_;
    DirectExternalSource directExternalSources_;
    ScriptKind kind_ {};
    bool isASTlowered_ {};
    bool genAbcForExternalSource_ {false};
    ScriptExtension extension_ {};
    ETSNolintsCollectionMap etsnolintCollection_;
    util::ModuleInfo moduleInfo_;

    lexer::SourcePosition packageStartPosition_ {};
    compiler::CFG *cfg_;
    ArenaVector<varbinder::FunctionScope *> functionScopes_;
    std::unordered_map<std::string, std::unordered_set<std::string>> fileDependencies_;

private:
    ArenaMap<int32_t, varbinder::VarBinder *> varbinders_;
    ArenaVector<checker::Checker *> checkers_;
#ifndef NDEBUG
    uint32_t poisonValue_ {POISON_VALUE};
#endif
    ProgramFlags programFlags_ {};
};
}  // namespace ark::es2panda::parser

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::parser::ProgramFlags> : std::true_type {
};

}  // namespace enumbitops

#endif
