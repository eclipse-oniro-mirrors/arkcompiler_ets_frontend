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
#ifndef ES2PANDA_COMPILER_CORE_ETSCOMPILER_H
#define ES2PANDA_COMPILER_CORE_ETSCOMPILER_H

#include "compiler/core/ASTCompiler.h"

namespace ark::es2panda::compiler {

class ETSCompiler final : public AstCompiler {
public:
    ETSCompiler() = default;
/* CC-OFFNXT(G.PRE.02,G.PRE.09) name part*/
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_ETSCOMPILER_COMPILE_METHOD(_, nodeType) void Compile(const ir::nodeType *node) const override;
    AST_NODE_MAPPING(DECLARE_ETSCOMPILER_COMPILE_METHOD)
#undef DECLARE_ETSCOMPILER_COMPILE_METHOD
/* CC-OFFNXT(G.PRE.02,G.PRE.09) name part*/
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_ETSCOMPILER_COMPILE_METHOD(_, __, nodeType, ___) void Compile(const ir::nodeType *node) const override;
    AST_NODE_REINTERPRET_MAPPING(DECLARE_ETSCOMPILER_COMPILE_METHOD)
#undef DECLARE_ETSCOMPILER_COMPILE_METHOD

private:
    void GetDynamicNameParts(const ir::CallExpression *expr, ArenaVector<util::StringView> &parts) const;
    void CompileCastPrimitives(const ir::Expression *expr, checker::Type const *targetType) const;
    void CompileCast(const ir::TSAsExpression *expr, checker::Type const *targetType) const;
    void EmitCall(const ir::CallExpression *expr, compiler::VReg &calleeReg, checker::Signature *signature) const;
    bool HandleArrayTypeLengthProperty(const ir::MemberExpression *expr, ETSGen *etsg) const;
    bool HandleTopLevelGetter(const ir::Identifier *expr, ETSGen *etsg) const;
    bool HandleStaticProperties(const ir::MemberExpression *expr, ETSGen *etsg) const;
    void CompileArrayCreation(const ir::ArrayExpression *expr) const;
    void CompileTupleCreation(const ir::ArrayExpression *tupleInitializer) const;

    static bool CompileComputed(compiler::ETSGen *etsg, const ir::MemberExpression *expr);

    ETSGen *GetETSGen() const;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_CORE_ETSCOMPILER_H
