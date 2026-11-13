/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef PANDA_UNION_NUMERIC_SEMANTICS_TEST_H
#define PANDA_UNION_NUMERIC_SEMANTICS_TEST_H

#include "ir/astNode.h"
#include "util/options.h"
#include "test/utils/panda_executable_path_getter.h"

namespace ark::es2panda::gtests {

class UnionNumericSemanticsTest : public testing::Test {
public:
    UnionNumericSemanticsTest()
        : allocator_(EHeap::NewAllocator()),
          publicContext_ {std::make_unique<public_lib::Context>()},
          phaseManager_ {ScriptExtension::ATS, Allocator()},
          checker_ {Allocator(), diagnosticEngine_}
    {
    }

    ~UnionNumericSemanticsTest() override
    {
        delete publicContext_->phaseManager;
    }

    static void SetUpTestCase()
    {
        es2panda::ScopedAllocatorsManager::Initialize();
    }

    ArenaAllocator *Allocator()
    {
        return allocator_.get();
    }

    parser::Program *Program()
    {
        return publicContext_->parserProgram;
    }

    checker::ETSChecker *Checker()
    {
        return &checker_;
    }

    util::DiagnosticEngine &DiagnosticEngine()
    {
        return diagnosticEngine_;
    }

    void InitializeChecker(std::string_view fileName, std::string_view src)
    {
        InitializeChecker<parser::ETSParser, varbinder::ETSBinder, checker::ETSChecker, checker::ETSAnalyzer,
                          compiler::ETSCompiler, compiler::ETSGen, compiler::StaticRegSpiller,
                          compiler::ETSFunctionEmitter, compiler::ETSEmitter>(
            Span(test::utils::PandaExecutablePathGetter::Get()), fileName, src, &checker_);
    }

    template <typename Parser, typename VarBinder, typename Checker, typename Analyzer, typename AstCompiler,
              typename CodeGen, typename RegSpiller, typename FunctionEmitter, typename Emitter>
    void InitializeChecker(Span<const char *const> args, std::string_view fileName, std::string_view src,
                           checker::ETSChecker *checker)
    {
        auto options = std::make_unique<ark::es2panda::util::Options>(args[0], diagnosticEngine_);
        if (!options->Parse(args)) {
            return;
        }

        ark::Logger::ComponentMask mask {};
        mask.set(ark::Logger::Component::ES2PANDA);
        ark::Logger::InitializeStdLogging(options->LogLevel(), mask);

        Compiler compiler(options->GetExtension(), options->GetThread());
        SourceFile input(fileName, src, options->IsModule());
        compiler::CompilationUnit unit {input, *options, 0, options->GetExtension(), diagnosticEngine_};

        auto config = public_lib::ConfigImpl {};
        publicContext_->config = &config;
        publicContext_->config->options = &unit.options;
        publicContext_->input = unit.input.source;
        publicContext_->sourceFileName = unit.input.filePath;
        publicContext_->sourceFile = &unit.input;
        publicContext_->allocator = allocator_.get();

        auto parser = Parser(publicContext_.get(), static_cast<parser::ParserStatus>(unit.rawParserStatus));
        publicContext_->parser = &parser;

        publicContext_->PushChecker(checker);
        auto analyzer = Analyzer(checker);
        checker->SetAnalyzer(&analyzer);
        publicContext_->PushAnalyzer(publicContext_->GetChecker()->GetAnalyzer());

        auto emitter = Emitter(publicContext_.get());
        publicContext_->emitter = &emitter;
        publicContext_->diagnosticEngine = &diagnosticEngine_;
        parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
        parser::ImportCache<parser::CacheType::METADATA>::ActivateCache();
        auto phaseManager = new compiler::PhaseManager(publicContext_.get(), unit.ext, allocator_.get());
        publicContext_->phaseManager = phaseManager;

        parser.ParseGlobal();
        while (auto phase = publicContext_->phaseManager->NextPhase()) {
            if (!phase->Apply(publicContext_.get())) {
                return;
            }
        }
    }

    static checker::Type *FindClassType(varbinder::ETSBinder *varbinder, std::string_view className)
    {
        auto classDefs = varbinder->AsETSBinder()->GetRecordTable()->ClassDefinitions();
        auto baseClass = std::find_if(classDefs.begin(), classDefs.end(), [className](ir::ClassDefinition *cdef) {
            return cdef->Ident()->Name().Is(className);
        });
        if (baseClass == classDefs.end()) {
            return nullptr;
        }
        return (*baseClass)->TsType();
    }

    static checker::Type *FindTypeAlias(checker::ETSChecker *checker, std::string_view aliasName)
    {
        auto *foundVar =
            checker->Scope()->FindLocal(aliasName, varbinder::ResolveBindingOptions::ALL)->AsLocalVariable();
        if (foundVar == nullptr) {
            return nullptr;
        }
        return foundVar->Declaration()->Node()->AsTSTypeAliasDeclaration()->TypeAnnotation()->TsType();
    }

    NO_COPY_SEMANTIC(UnionNumericSemanticsTest);
    NO_MOVE_SEMANTIC(UnionNumericSemanticsTest);

protected:
    static constexpr uint8_t SIZE1 = 1;
    static constexpr uint8_t SIZE2 = 2;
    static constexpr uint8_t SIZE3 = 3;
    static constexpr uint8_t IDX0 = 0;
    static constexpr uint8_t IDX1 = 1;
    static constexpr uint8_t IDX2 = 2;

    checker::Type *UndefinedType()
    {
        return Checker()->GetGlobalTypesHolder()->GlobalETSUndefinedType();
    }

    checker::Type *ObjectType()
    {
        return Checker()->GetGlobalTypesHolder()->GlobalETSObjectType();
    }

    checker::Type *StringType()
    {
        return Checker()->GlobalBuiltinETSStringType();
    }

    checker::Type *MakeUnion(std::initializer_list<checker::Type *> members)
    {
        return Checker()->CreateETSUnionType(std::vector<checker::Type *> {members});
    }

    bool AssignableTo(checker::Type *source, checker::Type *target)
    {
        return Checker()->Relation()->IsAssignableTo(source, target);
    }

    bool CastableTo(checker::Type *source, checker::Type *target)
    {
        return Checker()->Relation()->IsCastableTo(source, target);
    }

private:
    ark::es2panda::EHeap::Scope eheapScope_;
    std::unique_ptr<ArenaAllocator> allocator_;
    std::unique_ptr<public_lib::Context> publicContext_;
    ark::es2panda::compiler::PhaseManager phaseManager_;
    util::DiagnosticEngine diagnosticEngine_;
    checker::ETSChecker checker_;
};

}  // namespace ark::es2panda::gtests
#endif  // PANDA_UNION_NUMERIC_SEMANTICS_TEST_H
