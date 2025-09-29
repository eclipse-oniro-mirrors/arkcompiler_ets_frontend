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

#ifndef PANDA_UNION_NORMALISATION_TEST_H
#define PANDA_UNION_NORMALISATION_TEST_H

#include "ir/astNode.h"
#include "util/options.h"

namespace ark::es2panda::gtests {

class UnionNormalizationTest : public testing::Test {
public:
    UnionNormalizationTest()
        : allocator_(EHeap::NewAllocator()),
          publicContext_ {std::make_unique<public_lib::Context>()},
          phaseManager_ {ScriptExtension::ETS, Allocator()},
          checker_ {Allocator(), diagnosticEngine_}
    {
    }

    ~UnionNormalizationTest() override
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

    void InitializeChecker(std::string_view fileName, std::string_view src)
    {
        InitializeChecker<parser::ETSParser, varbinder::ETSBinder, checker::ETSChecker, checker::ETSAnalyzer,
                          compiler::ETSCompiler, compiler::ETSGen, compiler::StaticRegSpiller,
                          compiler::ETSFunctionEmitter, compiler::ETSEmitter>(
            Span(test::utils::PandaExecutablePathGetter::Get()), fileName, src, &checker_);
    }

    template <typename CodeGen, typename RegSpiller, typename FunctionEmitter, typename Emitter, typename AstCompiler>
    public_lib::Context::CodeGenCb MakeCompileJob()
    {
        return [this](public_lib::Context *context, varbinder::FunctionScope *scope,
                      compiler::ProgramElement *programElement) -> void {
            RegSpiller regSpiller;
            AstCompiler astcompiler;
            CodeGen cg(allocator_.get(), &regSpiller, context, scope, programElement, &astcompiler);
            FunctionEmitter funcEmitter(&cg, programElement);
            funcEmitter.Generate();
        };
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
        parser::DeclarationCache::ActivateCache();
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

    NO_COPY_SEMANTIC(UnionNormalizationTest);
    NO_MOVE_SEMANTIC(UnionNormalizationTest);

protected:
    static constexpr uint8_t SIZE2 = 2;
    static constexpr uint8_t SIZE3 = 3;
    static constexpr uint8_t IDX0 = 0;
    static constexpr uint8_t IDX1 = 1;
    static constexpr uint8_t IDX2 = 2;

private:
    ark::es2panda::EHeap::Scope eheapScope_;
    std::unique_ptr<ArenaAllocator> allocator_;
    std::unique_ptr<public_lib::Context> publicContext_;
    ark::es2panda::compiler::PhaseManager phaseManager_;
    util::DiagnosticEngine diagnosticEngine_;
    checker::ETSChecker checker_;
};

}  // namespace ark::es2panda::gtests
#endif  // PANDA_UNION_NORMALISATION_TEST_H
