/*
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

#include "etsPrimitiveType.h"

#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ETSPrimitiveType::TransformChildren([[maybe_unused]] const NodeTransformer &cb,
                                         [[maybe_unused]] std::string_view const transformationName)
{
    TransformAnnotations(cb, transformationName);
}

void ETSPrimitiveType::Iterate(const NodeTraverser &cb) const
{
    IterateAnnotations(cb);
}

void ETSPrimitiveType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSPrimitiveType"}, {"annotations", AstDumper::Optional(Annotations())}});
}

void ETSPrimitiveType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);

    switch (GetPrimitiveType()) {
        case PrimitiveType::BYTE:
            dumper->Add("byte");
            break;
        case PrimitiveType::INT:
            dumper->Add("int");
            break;
        case PrimitiveType::LONG:
            dumper->Add("long");
            break;
        case PrimitiveType::SHORT:
            dumper->Add("short");
            break;
        case PrimitiveType::FLOAT:
            dumper->Add("float");
            break;
        case PrimitiveType::DOUBLE:
            dumper->Add("double");
            break;
        case PrimitiveType::BOOLEAN:
            dumper->Add("boolean");
            break;
        case PrimitiveType::CHAR:
            dumper->Add("char");
            break;
        case PrimitiveType::VOID:
            dumper->Add("void");
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }
}

void ETSPrimitiveType::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ETSPrimitiveType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSPrimitiveType::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *ETSPrimitiveType::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GlobalAnyType();
}

checker::VerifiedType ETSPrimitiveType::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

checker::Type *ETSPrimitiveType::GetType([[maybe_unused]] checker::ETSChecker *checker)
{
    switch (GetPrimitiveType()) {
        case PrimitiveType::BYTE: {
            SetTsType(checker->GlobalByteBuiltinType());
            return TsType();
        }
        case PrimitiveType::SHORT: {
            SetTsType(checker->GlobalShortBuiltinType());
            return TsType();
        }
        case PrimitiveType::INT: {
            SetTsType(checker->GlobalIntBuiltinType());
            return TsType();
        }
        case PrimitiveType::LONG: {
            SetTsType(checker->GlobalLongBuiltinType());
            return TsType();
        }
        case PrimitiveType::FLOAT: {
            SetTsType(checker->GlobalFloatBuiltinType());
            return TsType();
        }
        case PrimitiveType::DOUBLE: {
            SetTsType(checker->GlobalDoubleBuiltinType());
            return TsType();
        }
        case PrimitiveType::BOOLEAN: {
            SetTsType(checker->GlobalETSBooleanBuiltinType());
            return TsType();
        }
        case PrimitiveType::CHAR: {
            SetTsType(checker->GlobalCharBuiltinType());
            return TsType();
        }
        case PrimitiveType::VOID: {
            if (LIKELY(checker->CheckVoidAnnotation(this))) {
                SetTsType(checker->GlobalVoidType());
                return TsType();
            }

            return checker->InvalidateType(this);
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

ETSPrimitiveType *ETSPrimitiveType::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<ETSPrimitiveType>(GetPrimitiveType(), allocator);
    ES2PANDA_ASSERT(clone != nullptr);

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir
