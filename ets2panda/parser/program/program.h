/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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
#include "util/ustring.h"
#include "util/path.h"
#include "util/importPathManager.h"
#include "util/enumbitops.h"
#include "ir/statements/blockStatement.h"

#include "lexer/token/sourceLocation.h"
#include "varbinder/varbinder.h"
#include "varbinder/recordTable.h"

#include "libarkbase/mem/pool_manager.h"
#include "libarkbase/os/filesystem.h"

#include <set>

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

#ifndef NDEBUG
constexpr uint32_t POISON_VALUE {0x12346789};
#endif

template <util::ModuleKind KIND>
class ProgramAdapter;

class RecordTableHolder {
public:
    void SetRecordTable(varbinder::RecordTable *recordTable)
    {
        recordTable_ = recordTable;
    }

    auto *GetRecordTable() const
    {
        return recordTable_;
    }

private:
    varbinder::RecordTable *recordTable_ {};
};

class Program : public RecordTableHolder {
    // To be moved from 'Program'.
    template <util::ModuleKind... KINDS>
    class ExternalSourcesImpl {
    public:
        template <util::ModuleKind KIND>
        using ProgramsSubmap = ArenaVector<ProgramAdapter<KIND> *>;
        using TransitiveExternals = std::tuple<ProgramsSubmap<KINDS>...>;

        explicit ExternalSourcesImpl() : transitiveExternals_(ProgramsSubmap<KINDS>()...), direct_ {} {}

        template <typename SubmapT>
        static constexpr auto GetModuleKindFromSubmapType()
        {
            using SubmapProgramT = std::remove_pointer_t<typename std::remove_reference_t<SubmapT>::value_type>;
            return SubmapProgramT::MODULE_KIND;
        }

        bool Empty() const
        {
            bool emptyTransitive = (std::get<ProgramsSubmap<KINDS>>(transitiveExternals_).empty() && ...);
            bool emptyDirect = direct_.empty();
            return emptyTransitive && emptyDirect;
        }

        template <util::ModuleKind KIND>
        const auto &Get() const
        {
            return std::get<ProgramsSubmap<KIND>>(transitiveExternals_);
        }

        template <util::ModuleKind KIND>
        auto &Get()
        {
            return std::get<ProgramsSubmap<KIND>>(transitiveExternals_);
        }

        template <typename ProgVisitor, util::ModuleKind SUBMAP_KIND>
        static constexpr bool INVOCABLE = std::is_invocable_v<ProgVisitor, ProgramAdapter<SUBMAP_KIND> *>;

        // Visits submaps selected by the following constraints:
        // - explicitly specified 'KINDS_TO_VISIT';
        // - callback parameter type.
        // NOTE(dkofanov): 'SHOULD_UNPACK_PACKAGE' should be removed when packages are merged.
        template <bool SHOULD_UNPACK_PACKAGE = true, util::ModuleKind... KINDS_TO_VISIT, typename ProgramVisitor>
        void Visit(const ProgramVisitor &cb)
        {
            static_assert(((INVOCABLE<ProgramVisitor, KINDS>) || ...),
                          "Visitor isn't invocable for any kind of programs");
            auto submapVisitor = [&cb](const auto &submap) {
                // CC-OFFNXT(G.NAM.03-CPP) project codestyle
                constexpr auto CUR_SUBMAP_KIND = GetModuleKindFromSubmapType<decltype(submap)>();
                // NOTE(dkofanov): Packages are to be removed from common externals.
                if constexpr (SHOULD_UNPACK_PACKAGE && (CUR_SUBMAP_KIND == util::ModuleKind::PACKAGE) &&
                              std::is_invocable_v<ProgramVisitor, SourceProgram *>) {
                    // "Unpack" package and iterate contents:
                    for (auto *pkg : submap) {
                        // As this is to be removed, do not handle variant with passing a key.
                        pkg->MaybeIteratePackage(cb);
                    }
                    return;
                }

                for (auto *prog : submap) {
                    ES2PANDA_ASSERT(prog->GetModuleKind() == CUR_SUBMAP_KIND);
                    if constexpr (INVOCABLE<ProgramVisitor, CUR_SUBMAP_KIND>) {
                        cb(prog);
                    }
                }
            };
            VisitSubmaps<KINDS_TO_VISIT...>(submapVisitor);
        }

        // This shouldn't try insert package fractions to packages.
        void Add(Program *progToInsert)
        {
            auto inserter = [progToInsert](auto &submap) {
                // CC-OFFNXT(G.NAM.03-CPP) project code style
                constexpr auto SUBMAP_KIND = GetModuleKindFromSubmapType<decltype(submap)>();
                if (progToInsert->Is<SUBMAP_KIND>()) {
                    submap.push_back(progToInsert->As<SUBMAP_KIND>());
                }
            };

            VisitSubmaps(inserter);
        }

        auto &Direct()
        {
            return direct_;
        }

        const auto &Direct() const
        {
            return direct_;
        }

    private:
        template <util::ModuleKind... KINDS_TO_VISIT, typename SubmapVisitor>
        void VisitSubmaps(SubmapVisitor &cb)
        {
            if constexpr (sizeof...(KINDS_TO_VISIT) == 0) {
                ((cb(std::get<ProgramsSubmap<KINDS>>(transitiveExternals_))), ...);
            } else {
                ((cb(std::get<ProgramsSubmap<KINDS_TO_VISIT>>(transitiveExternals_))), ...);
            }
        }

    private:
        TransitiveExternals transitiveExternals_;

        using DirectExternalPrograms = ArenaUnorderedMap<ArenaString, Program *>;
        DirectExternalPrograms direct_;

        friend Program;
    };

protected:
    Program(const util::ImportMetadata &importMetadata, ArenaAllocator *allocator, varbinder::VarBinder *varbinder);
    friend ArenaAllocator;

public:
    using ModuleKind = util::ModuleKind;

    // NOTE(dkofanov): 'ModuleKind::PACKAGE' should be replaced from here and stored there implicitly. They should be
    // merged at 'PackageImplicitImport' phase and added as just a 'ModuleKind::MODULE' with a single AST-tree.
    using ExternalSources = ExternalSourcesImpl<ModuleKind::MODULE, ModuleKind::SOURCE_DECL, ModuleKind::PACKAGE,
                                                ModuleKind::ETSCACHE_DECL>;

    using ETSNolintsCollectionMap = ArenaUnorderedMap<const ir::AstNode *, ArenaSet<ETSWarnings>>;

    template <util::ModuleKind KIND = util::ModuleKind::MODULE, typename VarBinderT = void>
    static ProgramAdapter<KIND> *New(const util::ImportMetadata &importMetadata, public_lib::Context *context);

    virtual ~Program();

    NO_COPY_SEMANTIC(Program);
    NO_MOVE_SEMANTIC(Program);

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

    const util::ImportMetadata &GetImportMetadata() const
    {
        return importMetadata_;
    }

    // NOTE(dkofanov): this function is not needed as soon as packages are merged.
    // They should be merged at PackageImplicitImport stage, but for now it handles only main-program.
    template <typename CB>
    void MaybeIteratePackage(const CB &cb);

    ScriptExtension Extension() const
    {
        return extension_;
    }

    std::string_view SourceCode() const
    {
        return sourceCode_;
    }

    util::StringView SourceFilePath() const
    {
        return sourceFile_.GetPath();
    }

    const util::Path &SourceFile() const
    {
        return sourceFile_;
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

    std::string RelativeFilePath(const public_lib::Context *context) const;

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
        VerifyDeclarationModule();
    }

    ir::ClassDefinition *GlobalClass();

    const ir::ClassDefinition *GlobalClass() const;

    void SetGlobalClass(ir::ClassDefinition *globalClass);

    ExternalSources *GetExternalSources()
    {
        return &externalSources_;
    }

    const ExternalSources *GetExternalSources() const
    {
        return &externalSources_;
    }

    // NOTE(dkofanov): this should be called exactly once. It is needed as soon as there is a special "main"-program and
    // others are "external-sources".
    void PromoteToMainProgram(public_lib::Context *ctx);

    const lexer::SourcePosition &PackageStart() const
    {
        return packageStartPosition_;
    }

    void SetPackageStart(const lexer::SourcePosition &start)
    {
        packageStartPosition_ = start;
    }

    void SetSource(const ark::es2panda::SourceFile &sourceFile)
    {
        sourceCode_ = sourceFile.source;
        sourceFile_ = util::Path(sourceFile.filePath, Allocator());
        moduleInfo_.isDeclForDynamicStaticInterop = sourceFile.isDeclForDynamicStaticInterop;
    }

    void SetPackageInfo(std::string_view mname, util::ModuleKind kind);

    const util::ModuleInfo &ModuleInfo() const
    {
        return moduleInfo_;
    }

    std::string_view ModuleName() const
    {
        return moduleInfo_.moduleName;
    }

    std::string_view ModulePrefix() const
    {
        return moduleInfo_.modulePrefix;
    }

    virtual util::ModuleKind GetModuleKind() const
    {
        // NOTE(dkofanov): this should be pure virtual, but now Program is exposed to the C-API
        // Result of this method is different from ModuleInfo::kind_, and tries to replace it in future.
        return util::ModuleKind::UNKNOWN;
    }

    template <util::ModuleKind KIND>
    bool Is() const
    {
        return GetModuleKind() == KIND;
    }

    template <util::ModuleKind KIND>
    ProgramAdapter<KIND> *As()
    {
        ES2PANDA_ASSERT(Is<KIND>());
        return static_cast<ProgramAdapter<KIND> *>(this);
    }

    bool IsDeclForDynamicStaticInterop() const
    {
        return moduleInfo_.isDeclForDynamicStaticInterop;
    }

    bool IsDeclarationModule() const
    {
        return Is<util::ModuleKind::SOURCE_DECL>() || Is<util::ModuleKind::ETSCACHE_DECL>();
    }

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

    void SetProgramModified(bool isModified)
    {
        isModified_ = isModified;
    }

    bool IsProgramModified() const
    {
        return isModified_;
    }

    bool IsStdLib() const
    {
        // NOTE (hurton): temporary solution, needs rework when std sources are renamed
        return (ModuleName().rfind("std.", 0) == 0) || (ModuleName().rfind("escompat", 0) == 0) ||
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

    auto &GetFileDependencies()
    {
        return fileDependencies_;
    }

    void AddFileDependencies(std::string_view file, std::string_view depFile)
    {
        fileDependencies_[ArenaString {file}].emplace(depFile);
    }

    ArenaMap<int32_t, varbinder::VarBinder *> &VarBinders()
    {
        return varbinders_;
    }

private:
    void VerifyDeclarationModule();

public:
    using FileDependenciesMap = ArenaUnorderedMap<ArenaString, ArenaUnorderedSet<ArenaString>>;

private:
    util::ImportMetadata importMetadata_;
    ArenaAllocator *allocator_ {};
    ir::BlockStatement *ast_ {};
    util::Path sourceFile_;
    std::string_view sourceCode_ {};

    bool isASTlowered_ {};
    bool isModified_ {true};
    bool genAbcForExternalSource_ {false};
    ScriptExtension extension_ {};
    ETSNolintsCollectionMap etsnolintCollection_;
    util::ModuleInfo moduleInfo_;
    lexer::SourcePosition packageStartPosition_ {};
    compiler::CFG *cfg_;

    FileDependenciesMap fileDependencies_;

    // NOTE(dkofanov): externalSources_ are stored only in main program. This field should be moved to
    // 'public_lib::Context'.
    ExternalSources externalSources_;

private:
    ArenaMap<int32_t, varbinder::VarBinder *> varbinders_;
    ArenaVector<checker::Checker *> checkers_;
#ifndef NDEBUG
    uint32_t poisonValue_ {POISON_VALUE};
#endif
    bool isAstChecked_ {false};
};

class NonPackageProgram : public Program {
public:
    using Program::Program;
};

template <util::ModuleKind KIND>
class ProgramAdapter final : public NonPackageProgram {
public:
    // CC-OFFNXT(G.NAM.03-CPP) project codestyle
    static constexpr auto MODULE_KIND = KIND;
    util::ModuleKind GetModuleKind() const override
    {
        return MODULE_KIND;
    }

    using NonPackageProgram::NonPackageProgram;
};

template <>
class ProgramAdapter<util::ModuleKind::PACKAGE> final : public Program {
public:
    static constexpr auto MODULE_KIND = util::ModuleKind::PACKAGE;
    using Program::Program;

    util::ModuleKind GetModuleKind() const override
    {
        return MODULE_KIND;
    }

    void AppendFraction(SourceProgram *fraction)
    {
        fractions_.push_back(fraction);
    }

    auto &GetUnmergedPackagePrograms()
    {
        return fractions_;
    }

private:
    ArenaVector<SourceProgram *> fractions_;
};

// NOTE(dkofanov): this function is not needed as soon as packages are merged.
// They should be merged at PackageImplicitImport stage, but for now it handles only main-program.
template <typename CB>
void Program::MaybeIteratePackage(const CB &cb)
{
    auto invokeMaybePassFlag = [&cb](auto *program, bool isPackageFraction) {
        // CC-OFFNXT(G.NAM.03-CPP) project codestyle
        constexpr bool SHOULD_INFORM_OF_PACKAGE_FRACTION = std::is_invocable_v<CB, Program *, bool>;
        if constexpr (SHOULD_INFORM_OF_PACKAGE_FRACTION) {
            cb(program, isPackageFraction);
        } else {
            cb(program);
        }
    };

    if (Is<util::ModuleKind::PACKAGE>()) {
        if (!As<util::ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms().empty()) {
            for (auto *fraction : As<util::ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms()) {
                ES2PANDA_ASSERT(fraction->Is<util::ModuleKind::MODULE>());
                invokeMaybePassFlag(fraction, true);
            }
        } else {
            // NOTE(dkofanov): merged packages are fractions too. They should be explicitly "depromoted" to
            // 'MODULE' in 'PackageImplicitImport'. As soon as this happens, the whole method should be deleted.
            invokeMaybePassFlag(this, true);
        }
    } else {
        invokeMaybePassFlag(this, false);
    }
}

}  // namespace ark::es2panda::parser

#endif
