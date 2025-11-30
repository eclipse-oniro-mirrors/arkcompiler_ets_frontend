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

#include "emitter.h"

#include <common/abc_file_utils.h>
#include <compiler/base/literals.h>
#include <compiler/core/compilerContext.h>
#include <compiler/core/pandagen.h>
#include <compiler/debugger/debuginfoDumper.h>
#include <compiler/base/catchTable.h>
#include <ir/base/annotation.h>
#include <ir/base/classDefinition.h>
#include <ir/base/property.h>
#include <ir/base/scriptFunction.h>
#include <ir/expressions/arrayExpression.h>
#include <ir/expressions/callExpression.h>
#include <ir/expressions/literals/booleanLiteral.h>
#include <ir/expressions/literals/numberLiteral.h>
#include <ir/expressions/literals/stringLiteral.h>
#include <ir/expressions/newExpression.h>
#include <ir/expressions/unaryExpression.h>
#include <ir/expressions/objectExpression.h>
#include <ir/statements/blockStatement.h>
#include <ir/statements/classDeclaration.h>
#include <ir/ts/tsArrayType.h>
#include <ir/ts/tsEnumMember.h>
#include <ir/ts/tsTypeParameterInstantiation.h>
#include <ir/ts/tsTypeReference.h>

namespace panda::es2panda::compiler {

FunctionEmitter::FunctionEmitter(ArenaAllocator *allocator, const PandaGen *pg)
    : pg_(pg),
      literalBuffers_(allocator->Adapter()),
      literalArrays_(allocator->Adapter()),
      externalAnnotationRecords_(allocator->Adapter())
{
    func_ = allocator->New<panda::pandasm::Function>(pg->InternalName().Mutf8(), pg->SourceLang());
    CHECK_NOT_NULL(func_);

    size_t paramCount = pg->InternalParamCount();
    func_->params.reserve(paramCount);

    for (uint32_t i = 0; i < paramCount; ++i) {
        func_->params.emplace_back(panda::pandasm::Type("any", 0), pg->SourceLang());
    }

    func_->regs_num = pg->TotalRegsNum();
    func_->return_type = panda::pandasm::Type("any", 0);
}

void FunctionEmitter::Generate(util::PatchFix *patchFixHelper)
{
    GenFunctionKind();
    GenIcSize();
    GenFunctionInstructions();
    GenVariablesDebugInfo();
    GenSourceFileDebugInfo();
    GenFunctionCatchTables();
    GenLiteralBuffers();
    GenConcurrentFunctionModuleRequests();
    if (pg_->Binder()->Program()->IsEnableAnnotations()) {
        GenAnnotations();
    }
    if (patchFixHelper != nullptr) {
        patchFixHelper->ProcessFunction(pg_, func_, literalBuffers_);
    }
    if (util::Helpers::IsEnableExpectedPropertyCountApiVersion(pg_->Binder()->Program()->TargetApiVersion())) {
        GenExpectedPropertyCountAnnotation();
    }
    GenSlotNumberAnnotation();
    if (pg_->Context()->IsMergeAbc()) {
        GenConcurrentModuleRequestsAnnotation();
    }
}

const ArenaSet<util::StringView> &FunctionEmitter::Strings() const
{
    return pg_->Strings();
}

void FunctionEmitter::GenFunctionKind()
{
    func_->SetFunctionKind(static_cast<panda::panda_file::FunctionKind>(pg_->GetFunctionKind()));
}

void FunctionEmitter::GenIcSize()
{
    func_->SetSlotsNum(pg_->GetCurrentSlot());
}

void FunctionEmitter::GenBufferLiterals(const LiteralBuffer *buff)
{
    Emitter::GenBufferLiterals(literalBuffers_, buff);
}

util::StringView FunctionEmitter::SourceCode() const
{
    if (pg_->RootNode()->IsProgram()) {
        return pg_->Binder()->Program()->SourceCode();
    }
    return static_cast<const ir::ScriptFunction *>(pg_->RootNode())->SourceCode(pg_->Binder());
}

lexer::LineIndex &FunctionEmitter::GetLineIndex() const
{
    return const_cast<lexer::LineIndex &>(pg_->Binder()->Program()->GetLineIndex());
}

static Format MatchFormat(const IRNode *node, const Formats &formats)
{
    std::array<const VReg *, IRNode::MAX_REG_OPERAND> regs {};
    auto regCnt = node->Registers(&regs);
    auto registers = Span<const VReg *>(regs.data(), regs.data() + regCnt);

    const auto *iter = formats.begin();

    for (; iter != formats.end(); iter++) {
        auto format = *iter;
        size_t limit = 0;
        for (const auto &formatItem : format.GetFormatItem()) {
            if (formatItem.IsVReg()) {
                limit = 1 << formatItem.Bitwidth();
                break;
            }
        }

        if (std::all_of(registers.begin(), registers.end(), [limit](const VReg *reg) { return *reg < limit; })) {
            return format;
        }
    }

    UNREACHABLE();
    return *iter;
}

static size_t GetIRNodeWholeLength(const IRNode *node)
{
    Formats formats = node->GetFormats();
    if (formats.empty()) {
        return 0;
    }

    size_t len = 1;
    constexpr size_t BIT_WIDTH = 8;
    const auto format = MatchFormat(node, formats);

    for (auto fi : format.GetFormatItem()) {
        len += fi.Bitwidth() / BIT_WIDTH;
    }

    return len;
}

[[maybe_unused]] static std::string WholeLine(const util::StringView &source, lexer::SourceRange range)
{
    return source.Substr(range.start.index, range.end.index).EscapeSymbol<util::StringView::Mutf8Encode>();
}

// This is for supporting setting breakpoint on the right parenthesis of a scriptFunciton.
uint32_t FunctionEmitter::UpdateForReturnIns(const ir::AstNode *astNode, panda::pandasm::Ins *pandaIns)
{
    constexpr size_t INVALID_LINE = -1;
    constexpr uint32_t INVALID_COL = -1;
    // The GetLocation method calculated position starts with 1, and
    // the column number in pandaIns->ins_debug starts with 0
    constexpr uint32_t OFFSET_COL = 1;
    uint32_t columnNum = INVALID_COL;
    if (pandaIns->opcode == pandasm::Opcode::RETURNUNDEFINED || pandaIns->opcode == pandasm::Opcode::RETURN) {
        while (astNode != nullptr && !astNode->IsScriptFunction()) {
            if (astNode->IsBlockStatement() &&
                astNode->AsBlockStatement()->Scope() &&
                astNode->AsBlockStatement()->Scope()->Node() &&
                astNode->AsBlockStatement()->Scope()->Node()->IsScriptFunction()) {
                astNode = astNode->AsBlockStatement()->Scope()->Node();
                break;
            }
            astNode = astNode->Parent();
        }
        pandaIns->ins_debug.line_number = astNode ? astNode->Range().end.line : INVALID_LINE;
        columnNum = astNode ? (GetLineIndex().GetLocation(astNode->Range().end).col - OFFSET_COL) : INVALID_COL;
    } else {
        pandaIns->ins_debug.line_number = astNode ? astNode->Range().start.line : INVALID_LINE;
        columnNum = astNode ? (GetLineIndex().GetLocation(astNode->Range().start).col - OFFSET_COL) : INVALID_COL;
    }
    return columnNum;
}

bool FunctionEmitter::NeedToAddColumnForPandaIns(panda::pandasm::Ins *pandaIns)
{
    if (pg_->IsDebug()) {
        return true;
    }
    // In other mode, adds column numbers to the call instructions can include the column numbers in the backstack info
    // of the 'not a callable' type errors.
    return pg_->EnableColumn() && (pandaIns->opcode == pandasm::Opcode::CALLTHIS0 ||
        pandaIns->opcode == pandasm::Opcode::CALLTHIS1 || pandaIns->opcode == pandasm::Opcode::CALLTHIS2 ||
        pandaIns->opcode == pandasm::Opcode::CALLTHIS3 || pandaIns->opcode == pandasm::Opcode::CALLTHISRANGE ||
        pandaIns->opcode == pandasm::Opcode::WIDE_CALLTHISRANGE ||
        pandaIns->opcode == pandasm::Opcode::CALLARG0 || pandaIns->opcode == pandasm::Opcode::CALLARG1 ||
        pandaIns->opcode == pandasm::Opcode::CALLARGS2 || pandaIns->opcode == pandasm::Opcode::CALLARGS3 ||
        pandaIns->opcode == pandasm::Opcode::CALLRANGE || pandaIns->opcode == pandasm::Opcode::WIDE_CALLRANGE);
}

void FunctionEmitter::GenInstructionDebugInfo(const IRNode *ins, panda::pandasm::Ins *pandaIns)
{
    const ir::AstNode *astNode = ins->Node();
    if (astNode == FIRST_NODE_OF_FUNCTION) {
        astNode = pg_->Debuginfo().firstStmt;
        if (!astNode) {
            return;
        }
    }
    uint32_t columnNum = UpdateForReturnIns(astNode, pandaIns);

    if (NeedToAddColumnForPandaIns(pandaIns)) {
        size_t insLen = GetIRNodeWholeLength(ins);
        offset_ += insLen;
        pandaIns->ins_debug.column_number = columnNum;
    }
}

static std::vector<panda::pandasm::LiteralArray::Literal> ProcessNewExpressionInLiteralArray(
    const ir::Expression *array)
{
    std::vector<panda::pandasm::LiteralArray::Literal> literals;

    const auto &typeParams = array->AsNewExpression()->TypeParams()->Params();
    auto type = typeParams[0]->Type();
    literals.emplace_back(
        pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::TAGVALUE,
                                        static_cast<uint8_t>(panda::panda_file::LiteralTag::BUILTINTYPEINDEX)});
    switch (type) {
        case ir::AstNodeType::TS_NUMBER_KEYWORD: {
            literals.emplace_back(pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                              ir::Annotation::EMPTY_LITERAL_ARRAY_WITH_NUMBER_TYPE});
            break;
        }
        case ir::AstNodeType::TS_BOOLEAN_KEYWORD: {
            literals.emplace_back(pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                              ir::Annotation::EMPTY_LITERAL_ARRAY_WITH_BOOLEAN_TYPE});
            break;
        }
        case ir::AstNodeType::TS_STRING_KEYWORD: {
            literals.emplace_back(pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                              ir::Annotation::EMPTY_LITERAL_ARRAY_WITH_STRING_TYPE});
            break;
        }
        case ir::AstNodeType::TS_TYPE_REFERENCE: {
            const auto &args = array->AsNewExpression()->Arguments();
            uint8_t value = args[0]->AsNumberLiteral()->Number<uint8_t>();
            if (value == ir::Annotation::ENUM_LITERAL_ARRAY_WITH_EMPTY_INITIALIZER_NUMBER_TYPE) {
                // By convention, this is an empty array of enums with underlying number type
                literals.emplace_back(pandasm::LiteralArray::Literal {
                    panda::panda_file::LiteralTag::BUILTINTYPEINDEX,
                    ir::Annotation::EMPTY_LITERAL_ARRAY_WITH_NUMBER_TYPE});
            } else if (value == ir::Annotation::ENUM_LITERAL_ARRAY_WITH_EMPTY_INITIALIZER_STRING_TYPE) {
                // By convention, this is an empty array of enums with underlying string type
                literals.emplace_back(pandasm::LiteralArray::Literal {
                    panda::panda_file::LiteralTag::BUILTINTYPEINDEX,
                    ir::Annotation::EMPTY_LITERAL_ARRAY_WITH_STRING_TYPE});
            } else {
                UNREACHABLE();
            }
            break;
        }
        default:
            UNREACHABLE();
    }

    return literals;
}

static std::vector<panda::pandasm::LiteralArray::Literal> ProcessArrayExpressionInLiteralArray(
    const ir::Expression *array, const std::string &baseName,
    std::vector<std::pair<std::string, std::vector<Literal>>> &result)
{
    std::vector<panda::pandasm::LiteralArray::Literal> literals;
    ArenaVector<ir::Expression *> elements {array->AsArrayExpression()->Elements()};

    for (const auto *elem : elements) {
        if (elem->IsArrayExpression() || elem->IsNewExpression()) {
            auto litArrays = Emitter::CreateLiteralArray(elem, baseName);
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::TAGVALUE,
                                                static_cast<uint8_t>(panda::panda_file::LiteralTag::LITERALARRAY)});
            literals.emplace_back(pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::LITERALARRAY,
                                                                  litArrays.back().first});
            for (auto item : litArrays) {
                result.push_back(item);
            }
        } else if (elem->IsNumberLiteral()) {
            double doubleValue = elem->AsNumberLiteral()->Number();
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::TAGVALUE,
                                                static_cast<uint8_t>(panda::panda_file::LiteralTag::DOUBLE)});
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::DOUBLE, doubleValue});
        } else if (elem->IsBooleanLiteral()) {
            bool boolValue = elem->AsBooleanLiteral()->Value();
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::TAGVALUE,
                                                static_cast<uint8_t>(panda::panda_file::LiteralTag::BOOL)});
            literals.emplace_back(pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::BOOL, boolValue});
        } else if (elem->IsStringLiteral()) {
            std::string stringValue {elem->AsStringLiteral()->Str().Utf8()};
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::TAGVALUE,
                                                static_cast<uint8_t>(panda::panda_file::LiteralTag::STRING)});
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::STRING, stringValue});
        } else if (elem->IsUnaryExpression()) {
            ASSERT(elem->AsUnaryExpression()->IsNegativeNumber());
            // In annotations other unary operators (+, !, ~) are evaluated in tsc.
            // Only unary minus may be present in intermediate .ts
            double doubleValue = (-1) * elem->AsUnaryExpression()->Argument()->AsNumberLiteral()->Number();
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::TAGVALUE,
                                                static_cast<uint8_t>(panda::panda_file::LiteralTag::DOUBLE)});
            literals.emplace_back(
                pandasm::LiteralArray::Literal {panda::panda_file::LiteralTag::DOUBLE, doubleValue});
        } else {
            UNREACHABLE();
        }
    }

    return literals;
}

std::vector<std::pair<std::string, std::vector<Literal>>> Emitter::CreateLiteralArray(const ir::Expression *array,
                                                                                      const std::string &baseName)
{
    std::vector<std::pair<std::string, std::vector<Literal>>> result;
    std::vector<panda::pandasm::LiteralArray::Literal> literals;

    if (array->IsNewExpression()) {
        // special case, when initializer is an empty array
        literals = ProcessNewExpressionInLiteralArray(array);
    } else {
        literals = ProcessArrayExpressionInLiteralArray(array, baseName, result);
    }

    static uint32_t litArrayValueCount = 0;
    std::string litArrayName = baseName + "_" + std::to_string(litArrayValueCount);
    ++litArrayValueCount;
    result.push_back(std::make_pair(litArrayName, literals));
    return result;
}

pandasm::AnnotationElement FunctionEmitter::CreateAnnotationElement(const std::string &propName,
                                                                    const ir::Expression *initValue)
{
    if (initValue->IsArrayExpression() || initValue->IsNewExpression()) {
        std::string baseName {func_->name + "_" + propName};
        auto litArrays = Emitter::CreateLiteralArray(initValue, baseName);
        for (auto item : litArrays) {
            literalArrays_.push_back(item);
        }
        return pandasm::AnnotationElement {
            propName,
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::LITERALARRAY>(
                std::string_view {litArrays.back().first}))};
    } else if (initValue->IsNumberLiteral()) {
        return pandasm::AnnotationElement {
            propName, std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::F64>(
                          initValue->AsNumberLiteral()->Number()))};
    } else if (initValue->IsBooleanLiteral()) {
        return pandasm::AnnotationElement {
            propName, std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::U1>(
                          initValue->AsBooleanLiteral()->Value()))};
    } else if (initValue->IsStringLiteral()) {
        std::string_view stringValue {initValue->AsStringLiteral()->Str().Utf8()};
        return pandasm::AnnotationElement {
            propName, std::make_unique<pandasm::ScalarValue>(
                          pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(stringValue))};
    } else if (initValue->IsUnaryExpression()) {
        ASSERT(initValue->AsUnaryExpression()->IsNegativeNumber());
        // In annotations other unary operators (+, !, ~) are evaluated in tsc.
        // Only unary minus may be present in intermediate .ts
        double negNumberValue = (-1) * initValue->AsUnaryExpression()->Argument()->AsNumberLiteral()->Number();
        return pandasm::AnnotationElement {
            propName, std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::F64>(
                          negNumberValue))};
    } else {
        UNREACHABLE();
    }
}

static pandasm::Record CreateExternalAnnotationRecord(const std::string &name, pandasm::extensions::Language lang)
{
    pandasm::Record record(name, lang);
    record.metadata->SetAccessFlags(panda::ACC_ANNOTATION);
    record.metadata->SetAttribute("external");
    return record;
}

pandasm::AnnotationData FunctionEmitter::CreateAnnotation(const ir::Annotation *anno)
{
    std::string annoName = std::string(anno->Name());
    if (pg_->Context()->IsMergeAbc()) {
        std::string prefix =
            std::string(pg_->Context()->RecordName()) + std::string(abc2program::RECORD_ANNOTATION_SEPARATOR);
        annoName.insert(0, prefix);
    }
    pandasm::AnnotationData annotation(annoName);

    if (anno->IsImported()) {
        externalAnnotationRecords_.push_back(
            CreateExternalAnnotationRecord(annoName, pg_->SourceLang()));
    }

    if (!anno->Expr()->IsCallExpression()) {
        [[maybe_unused]] auto checkExpr = anno->Expr()->IsIdentifier() || anno->Expr()->IsMemberExpression();
        ASSERT(checkExpr == true);
        return annotation;
    }

    const ir::CallExpression *callExpr = anno->Expr()->AsCallExpression();
    if (callExpr->Arguments().size() == 0) {
        return annotation;
    }

    // By convention, all properties are initialized in TSC, so that we don't need to process
    // default arguments in annotation interface declaration
    for (auto prop : callExpr->Arguments()[0]->AsObjectExpression()->Properties()) {
        std::string propName {prop->AsProperty()->Key()->AsIdentifier()->Name()};
        const ir::Expression *initValue = prop->AsProperty()->Value();
        annotation.AddElement(CreateAnnotationElement(propName, initValue));
    }

    return annotation;
}

void FunctionEmitter::GenAnnotations()
{
    if (!pg_->RootNode()->IsScriptFunction()) {
        return;
    }

    auto *scriptFunction = pg_->RootNode()->AsScriptFunction();
    if (!scriptFunction->Parent() || !scriptFunction->Parent()->Parent() ||
        !scriptFunction->Parent()->Parent()->IsMethodDefinition()) {
        return;
    }

    auto *methodDefinition = scriptFunction->Parent()->Parent()->AsMethodDefinition();
    std::vector<pandasm::AnnotationData> annotations;
    for (auto *anno : methodDefinition->Annotations()) {
        annotations.emplace_back(CreateAnnotation(anno));
    }

    func_->metadata->AddAnnotations(annotations);
}

void FunctionEmitter::GenFunctionInstructions()
{
    func_->ins.reserve(pg_->Insns().size());

    for (const auto *ins : pg_->Insns()) {
        auto *pandaIns = ins->Transform();
        func_->ins.emplace_back(pandaIns);
        GenInstructionDebugInfo(ins, pandaIns);
    }
}

void FunctionEmitter::GenFunctionCatchTables()
{
    func_->catch_blocks.reserve(pg_->CatchList().size());

    for (const auto *catchBlock : pg_->CatchList()) {
        const auto &labelSet = catchBlock->LabelSet();

        auto &pandaCatchBlock = func_->catch_blocks.emplace_back();
        pandaCatchBlock.try_begin_label = labelSet.TryBegin()->Id();
        pandaCatchBlock.try_end_label = labelSet.TryEnd()->Id();
        pandaCatchBlock.catch_begin_label = labelSet.CatchBegin()->Id();
        pandaCatchBlock.catch_end_label = labelSet.CatchEnd()->Id();
    }
}

void FunctionEmitter::GenLiteralBuffers()
{
    for (const auto *buff : pg_->BuffStorage()) {
        GenBufferLiterals(buff);
    }
}

void FunctionEmitter::GenSourceFileDebugInfo()
{
    if (pg_->SourceFile().empty()) {
        func_->source_file = std::string {pg_->Binder()->Program()->SourceFile()};
    } else {
        func_->source_file = pg_->SourceFile();
    }

    if (!pg_->IsDebug()) {
        return;
    }

    if (pg_->RootNode()->IsProgram()) {
        if (pg_->Context()->IsRecordDebugSource()) {
            func_->source_code = SourceCode().Mutf8();
        } else {
            func_->source_code = "not supported";
        }
    }
}

void FunctionEmitter::GenScopeVariableInfo(const binder::Scope *scope)
{
    const auto *startIns = scope->ScopeStart();
    const auto *endIns = scope->ScopeEnd();

    uint32_t start = 0;
    uint32_t count = 0;

    for (const auto *it : pg_->Insns()) {
        if (startIns == it) {
            start = count;
        } else if (endIns == it) {
            auto varsLength = static_cast<uint32_t>(count - start + 1);

            for (const auto &[name, variable] : scope->Bindings()) {
                if (!variable->IsLocalVariable() || variable->LexicalBound()) {
                    continue;
                }

                auto &variableDebug = func_->local_variable_debug.emplace_back();
                variableDebug.name = name.Mutf8();
                variableDebug.signature = "any";
                variableDebug.signature_type = "any";
                // Register spill causes an offset being applied to all registers in all instructions (refer to
                // RegAllocator::AdjustInsRegWhenHasSpill for more details). Therefore, we also add this offset
                // to the variable-to-register mapping before dumping it to the debug information of the abc file
                // (the following code).
                // We do not correct the original variable mapping in the scope object since it is not used after
                // the register spill phase, while we do not know the number of spilled registers before that phase.
                // If any future modifications require access to the variable mapping after the register spill,
                // please correct the mapping first and remove the offset increment operation in the following code.
                variableDebug.reg =
                    static_cast<int32_t>(variable->AsLocalVariable()->Vreg()) + pg_->GetSpillRegsCount();
                variableDebug.start = start;
                variableDebug.length = static_cast<uint32_t>(varsLength);
            }

            break;
        }

        count++;
    }
}

void FunctionEmitter::GenVariablesDebugInfo()
{
    if (!pg_->IsDebug()) {
        return;
    }

    for (const auto *scope : pg_->Debuginfo().variableDebugInfo) {
        GenScopeVariableInfo(scope);
    }
}

void FunctionEmitter::GenConcurrentFunctionModuleRequests()
{
    if (static_cast<panda::panda_file::FunctionKind>(pg_->GetFunctionKind()) !=
        panda::panda_file::FunctionKind::CONCURRENT_FUNCTION) {
        return;
    }

    std::vector<int> moduleRequests =
        static_cast<const ir::ScriptFunction *>(pg_->RootNode())->GetConcurrentModuleRequests();
    func_->concurrent_module_requests.reserve(moduleRequests.size());
    for (auto it : moduleRequests) {
        func_->concurrent_module_requests.emplace_back(it);
    }
}

void FunctionEmitter::GenExpectedPropertyCountAnnotation()
{
    auto expectedCount = pg_->GetExpectedPropertyCount();
    if (expectedCount == 0) {
        return;
    }

    static const std::string ANNOTATION = "_ESExpectedPropertyCountAnnotation";
    static const std::string ELEMENT_NAME = "ExpectedPropertyCount";

    pandasm::AnnotationData anno(ANNOTATION);

    pandasm::AnnotationElement ele(ELEMENT_NAME, std::make_unique<pandasm::ScalarValue>(
        pandasm::ScalarValue::Create<pandasm::Value::Type::U32>(expectedCount)));
    anno.AddElement(std::move(ele));

    std::vector<pandasm::AnnotationData> annos;
    annos.emplace_back(anno);
    func_->metadata->AddAnnotations(annos);
    func_->SetExpectedPropertyCount(pg_->GetExpectedPropertyCount());
}

void FunctionEmitter::GenSlotNumberAnnotation()
{
    static const std::string SLOT_NUMBER = "_ESSlotNumberAnnotation";
    static const std::string ELEMENT_NAME = "SlotNumber";

    for (const auto &an : func_->metadata->GetAnnotations()) {
        if (an.GetName() == SLOT_NUMBER) {
            return;
        }
    }
    pandasm::AnnotationData anno(SLOT_NUMBER);
    pandasm::AnnotationElement ele(ELEMENT_NAME, std::make_unique<pandasm::ScalarValue>(
        pandasm::ScalarValue::Create<pandasm::Value::Type::U32>(static_cast<uint32_t>(func_->GetSlotsNum()))));
    anno.AddElement(std::move(ele));
    std::vector<pandasm::AnnotationData> annos;
    annos.emplace_back(anno);
    func_->metadata->AddAnnotations(annos);
}

void FunctionEmitter::GenConcurrentModuleRequestsAnnotation()
{
    static const std::string CONCURRENT_MODULE_REQUESTS = "_ESConcurrentModuleRequestsAnnotation";
    static const std::string ELEMENT_NAME = "ConcurrentModuleRequest";
    if (func_->GetFunctionKind() != panda::panda_file::FunctionKind::CONCURRENT_FUNCTION) {
        return;
    }

    for (const auto &an : func_->metadata->GetAnnotations()) {
        if (an.GetName() == CONCURRENT_MODULE_REQUESTS) {
            return;
        }
    }

    pandasm::AnnotationData anno(CONCURRENT_MODULE_REQUESTS);
    for (auto &it : func_->concurrent_module_requests) {
        panda::pandasm::AnnotationElement module_request(ELEMENT_NAME, std::make_unique<pandasm::ScalarValue>(
            pandasm::ScalarValue::Create<pandasm::Value::Type::U32>(static_cast<uint32_t>(it))));
        anno.AddElement(std::move(module_request));
    }

    std::vector<pandasm::AnnotationData> annos;
    annos.emplace_back(anno);
    func_->metadata->AddAnnotations(annos);
}

// Emitter
Emitter::Emitter(CompilerContext *context): source_lang_(context->SourceLang())
{
    prog_ = new panda::pandasm::Program();
    prog_->lang = context->SourceLang();

    if (context->IsJsonInputFile()) {
        GenJsonContentRecord(context);
        return;
    }

    if (context->IsMergeAbc()) {
        auto recordName = context->Binder()->Program()->FormatedRecordName().Mutf8();
        rec_ = new panda::pandasm::Record(recordName.substr(0, recordName.find_last_of('.')),
                                          prog_->lang);
        SetPkgNameField(context->PkgName());
        SetCommonjsField(context->Binder()->Program()->Kind() == parser::ScriptKind::COMMONJS);
    } else {
        rec_ = nullptr;
        if (context->Binder()->Program()->Kind() == parser::ScriptKind::COMMONJS) {
            GenCommonjsRecord();
        }
    }
    if (context->Binder()->Program()->Kind() == parser::ScriptKind::MODULE) {
        AddHasTopLevelAwaitRecord(context->Binder()->Program()->HasTLA(), context);
    }
    AddSharedModuleRecord(context);
    AddScopeNamesRecord(context);
    if (util::Helpers::IsEnableExpectedPropertyCountApiVersion(context->Binder()->Program()->TargetApiVersion())) {
        AddExpectedPropertyCountRecord();
    }
    AddSlotNumberRecord();
    if (context->IsMergeAbc()) {
        AddConcurrentModuleRequestsRecord();
    }
}

Emitter::~Emitter()
{
    delete prog_;
}

void Emitter::SetPkgNameField(const std::string &pkgName)
{
    auto pkgNameField = panda::pandasm::Field(source_lang_);
    pkgNameField.name = "pkgName@" + pkgName;
    pkgNameField.type = panda::pandasm::Type("u8", 0);
    pkgNameField.metadata->SetValue(
        panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::U8>(static_cast<uint8_t>(0)));
    rec_->field_list.emplace_back(std::move(pkgNameField));
}

void Emitter::GenRecordNameInfo() const
{
    if (rec_) {
        prog_->record_table.emplace(rec_->name, std::move(*rec_));
    }
}

void Emitter::GenJsonContentRecord(const CompilerContext *context)
{
    rec_ = new panda::pandasm::Record(std::string(context->RecordName()), source_lang_);
    auto jsonContentField = panda::pandasm::Field(source_lang_);
    jsonContentField.name = "jsonFileContent";
    jsonContentField.type = panda::pandasm::Type("u32", 0);
    jsonContentField.metadata->SetValue(panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::STRING>(
        static_cast<std::string_view>(context->SourceFile())));
    rec_->field_list.emplace_back(std::move(jsonContentField));
    if (context->PatchFixHelper()) {
        context->PatchFixHelper()->ProcessJsonContentRecord(rec_->name, context->SourceFile());
    }
}

void Emitter::AddFunction(FunctionEmitter *func, CompilerContext *context)
{
    std::lock_guard<std::mutex> lock(m_);

    for (const auto &str : func->Strings()) {
        prog_->strings.insert(str.Mutf8());
    }

    for (auto &[idx, buf] : func->LiteralBuffers()) {
        auto literalArrayInstance = panda::pandasm::LiteralArray(std::move(buf));
        auto litId = std::string(context->Binder()->Program()->RecordName()) + "_" + std::to_string(idx);
        prog_->literalarray_table.emplace(litId, std::move(literalArrayInstance));
    }

    for (auto &[name, buf] : func->LiteralArrays()) {
        auto literalArrayInstance = panda::pandasm::LiteralArray(std::move(buf));
        prog_->literalarray_table.emplace(name, std::move(literalArrayInstance));
    }

    for (auto &&rec : func->ExternalAnnotationRecords()) {
        prog_->record_table.emplace(rec.name, std::move(rec));
    }

    auto *function = func->Function();
    prog_->function_table.emplace(function->name, std::move(*function));
}

void Emitter::AddScopeNamesRecord(CompilerContext *context)
{
    std::lock_guard<std::mutex> lock(m_);
    // make literalarray for scope names
    if (util::Helpers::IsDefaultApiVersion(context->Binder()->Program()->TargetApiVersion(),
        context->Binder()->Program()->GetTargetApiSubVersion())) {
        return;
    }
    const auto &scopeNamesMap = context->Binder()->GetScopeNames();

    panda::pandasm::LiteralArray array;
    const int32_t DOUBLE_SIZE = 2;
    array.literals_.resize(scopeNamesMap.size() * DOUBLE_SIZE);
    auto strTag = panda_file::LiteralTag::STRING;
    for (const auto &[scopeName, index] : scopeNamesMap) {
        panda::pandasm::LiteralArray::Literal tag;
        tag.tag_ = panda_file::LiteralTag::TAGVALUE;
        tag.value_ = static_cast<uint8_t>(strTag);
        array.literals_.at(index * DOUBLE_SIZE) = std::move(tag);
        panda::pandasm::LiteralArray::Literal val;
        val.tag_ = strTag;
        val.value_ = std::string(scopeName);
        array.literals_.at(index * DOUBLE_SIZE + 1) = std::move(val);
    }
    auto literalKey =
        std::string(context->Binder()->Program()->RecordName()) + "_" + std::to_string(context->NewLiteralIndex());
    prog_->literalarray_table.emplace(literalKey, std::move(array));

    // ScopeNames is a literalarray in each record if it is in mergeAbc, it is a string array which put scope names.
    // _ESScopeNamesRecord is a literalarray in the record when it is not in mergeAbc.
    if (context->IsMergeAbc()) {
        auto scopeNamesField = panda::pandasm::Field(source_lang_);
        scopeNamesField.name = "scopeNames";
        scopeNamesField.type = panda::pandasm::Type("u32", 0);
        scopeNamesField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
                static_cast<std::string_view>(literalKey)));
        rec_->field_list.emplace_back(std::move(scopeNamesField));
    } else {
        auto scopeNamesRecord = panda::pandasm::Record("_ESScopeNamesRecord",
                                                       source_lang_);
        scopeNamesRecord.metadata->SetAccessFlags(panda::ACC_PUBLIC);
        auto scopeNamesField = panda::pandasm::Field(source_lang_);
        // If the input arg "source-file" is not specified, context->SourceFile() will be empty,
        // in this case, use it's absolute path.
        if (context->SourceFile().empty()) {
            scopeNamesField.name = context->Binder()->Program()->SourceFile().Mutf8();
        } else {
            scopeNamesField.name = context->SourceFile();
        }
        scopeNamesField.type = panda::pandasm::Type("u32", 0);
        scopeNamesField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
                static_cast<std::string_view>(literalKey)));
        scopeNamesRecord.field_list.emplace_back(std::move(scopeNamesField));
        prog_->record_table.emplace(scopeNamesRecord.name, std::move(scopeNamesRecord));
    }
}

void Emitter::CreateStringClass()
{
    if (prog_->record_table.find(ir::Annotation::stringClassName) == prog_->record_table.end()) {
        pandasm::Record record(ir::Annotation::stringClassName, prog_->lang);
        record.metadata->SetAttribute("external");
        prog_->record_table.emplace(ir::Annotation::stringClassName, std::move(record));
    }
}

panda::pandasm::Type Emitter::DeduceArrayEnumType(const ir::Expression *value, uint8_t rank,
                                                  bool &needToCreateArrayValue)
{
    // By convention, array of enum must always has initializer
    ASSERT(value != nullptr);
    while (value->IsArrayExpression()) {
        const auto &elements = value->AsArrayExpression()->Elements();
        value = elements[0];
    }

    if (value->IsNumberLiteral()) {
        return panda::pandasm::Type("f64", rank);
    } else if (value->IsStringLiteral()) {
        CreateStringClass();
        return panda::pandasm::Type(ir::Annotation::stringClassName, rank);
    } else if (value->IsNewExpression()) {
        const auto &args = value->AsNewExpression()->Arguments();
        uint8_t value = args[0]->AsNumberLiteral()->Number<uint8_t>();
        switch (value) {
            // By convention, this is an array of enums with underlying number type without initializer
            case ir::Annotation::ENUM_LITERAL_ARRAY_WITHOUT_INITIALIZER_NUMBER_TYPE: {
                needToCreateArrayValue = false;
                return panda::pandasm::Type("f64", rank);
            }
            case ir::Annotation::ENUM_LITERAL_ARRAY_WITH_EMPTY_INITIALIZER_NUMBER_TYPE: {
                // By convention, this is an array of enums with underlying number type with empty array initializer
                needToCreateArrayValue = true;
                return panda::pandasm::Type("f64", rank);
            }
            case ir::Annotation::ENUM_LITERAL_ARRAY_WITHOUT_INITIALIZER_STRING_TYPE: {
                // By convention, this is an array of enums with underlying string type without initializer
                needToCreateArrayValue = false;
                CreateStringClass();
                return panda::pandasm::Type(ir::Annotation::stringClassName, rank);
            }
            case ir::Annotation::ENUM_LITERAL_ARRAY_WITH_EMPTY_INITIALIZER_STRING_TYPE: {
                // By convention, this is an array of enums with underlying string type with empty array initializer
                needToCreateArrayValue = true;
                CreateStringClass();
                return panda::pandasm::Type(ir::Annotation::stringClassName, rank);
            }
            default:
                UNREACHABLE();
        }
    }
    UNREACHABLE();
}

void Emitter::CreateLiteralArrayProp(const ir::ClassProperty *prop, const std::string &annoName,
                                     panda::pandasm::Field &annoRecordField)
{
    uint8_t rank = 1;
    auto *elemType = prop->TypeAnnotation()->AsTSArrayType()->ElementType();
    while (elemType->Type() == ir::AstNodeType::TS_ARRAY_TYPE) {
        ++rank;
        elemType = elemType->AsTSArrayType()->ElementType();
    }

    if (elemType->Type() == ir::AstNodeType::TS_NUMBER_KEYWORD) {
        annoRecordField.type = panda::pandasm::Type("f64", rank);
    } else if (elemType->Type() == ir::AstNodeType::TS_BOOLEAN_KEYWORD) {
        annoRecordField.type = panda::pandasm::Type("u1", rank);
    } else if (elemType->Type() == ir::AstNodeType::TS_STRING_KEYWORD) {
        CreateStringClass();
        annoRecordField.type = panda::pandasm::Type(ir::Annotation::stringClassName, rank);
    } else if (elemType->Type() == ir::AstNodeType::TS_TYPE_REFERENCE) {
        bool needToCreateArrayValue = true;
        annoRecordField.type = DeduceArrayEnumType(prop->Value(), rank, needToCreateArrayValue);
        if (!needToCreateArrayValue) {
            return;
        }
    } else {
        UNREACHABLE();
    }

    auto value = prop->Value();
    if (value != nullptr) {
        std::string baseName = annoName + "_" + annoRecordField.name;
        auto litArray = Emitter::CreateLiteralArray(value, baseName);
        for (auto item : litArray) {
            prog_->literalarray_table.emplace(item.first, pandasm::LiteralArray(item.second));
        }
        annoRecordField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
                std::string_view {litArray.back().first}));
    }
}

void Emitter::CreateEnumProp(const ir::ClassProperty *prop, const std::string &annoName,
                             panda::pandasm::Field &annoRecordField)
{
    auto value = prop->Value();
    if (value == nullptr) {
        // By convention, if annotation interface prop has no init value,
        // then underlying type of annotation is string
        CreateStringClass();
        annoRecordField.type = panda::pandasm::Type(ir::Annotation::stringClassName, 0);
    } else if (value->IsTSAsExpression()) {
        // By convention, if annotation interface prop has init value "new Number(0) as number",
        // then underlying type of annotation is number
        annoRecordField.type = panda::pandasm::Type("f64", 0);
    } else if (value->IsStringLiteral()) {
        CreateStringClass();
        annoRecordField.type = panda::pandasm::Type(ir::Annotation::stringClassName, 0);
        std::string_view stringValue {value->AsStringLiteral()->Str().Utf8()};
        annoRecordField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::STRING>(stringValue));
    } else if (value->IsNumberLiteral()) {
        annoRecordField.type = panda::pandasm::Type("f64", 0);
        double doubleValue = value->AsNumberLiteral()->Number();
        annoRecordField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::F64>(doubleValue));
    } else if (value->IsUnaryExpression()) {
        ASSERT(value->AsUnaryExpression()->IsNegativeNumber());
        // In annotations other unary operators (+, !, ~) are evaluated in tsc.
        // Only unary minus may be present in intermediate .ts
        annoRecordField.type = panda::pandasm::Type("f64", 0);
        double doubleValue = (-1) * value->AsUnaryExpression()->Argument()->AsNumberLiteral()->Number();
        annoRecordField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::F64>(doubleValue));
    } else {
        UNREACHABLE();
    }
}

panda::pandasm::Field Emitter::CreateAnnotationProp(const ir::ClassProperty *prop,
                                                    const std::string &annoName)
{
    auto annoRecordField = panda::pandasm::Field(source_lang_);
    annoRecordField.name = std::string(prop->Key()->AsIdentifier()->Name());

    auto propType = prop->TypeAnnotation()->Type();
    auto value = prop->Value();
    if (propType == ir::AstNodeType::TS_NUMBER_KEYWORD) {
        annoRecordField.type = panda::pandasm::Type("f64", 0);
        if (value != nullptr) {
            double doubleValue = 0.0;
            if (value->IsUnaryExpression()) {
                ASSERT(value->AsUnaryExpression()->IsNegativeNumber());
                // In annotations other unary operators (+, !, ~) are evaluated in tsc.
                // Only unary minus may be present in intermediate .ts
                value = value->AsUnaryExpression()->Argument();
                doubleValue = (-1) * value->AsNumberLiteral()->Number();
            } else {
                doubleValue = value->AsNumberLiteral()->Number();
            }
            annoRecordField.metadata->SetValue(
                panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::F64>(doubleValue));
        }
    } else if (propType == ir::AstNodeType::TS_BOOLEAN_KEYWORD) {
        annoRecordField.type = panda::pandasm::Type("u1", 0);
        if (value != nullptr) {
            bool boolValue = value->AsBooleanLiteral()->Value();
            annoRecordField.metadata->SetValue(
                panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::U1>(boolValue));
        }
    } else if (propType == ir::AstNodeType::TS_STRING_KEYWORD) {
        CreateStringClass();
        annoRecordField.type = panda::pandasm::Type(ir::Annotation::stringClassName, 0);
        if (value != nullptr) {
            std::string_view stringValue {value->AsStringLiteral()->Str().Utf8()};
            annoRecordField.metadata->SetValue(
                panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::STRING>(stringValue));
        }
    } else if (propType == ir::AstNodeType::TS_ARRAY_TYPE) {
        CreateLiteralArrayProp(prop, annoName, annoRecordField);
    } else if (propType == ir::AstNodeType::TS_TYPE_REFERENCE) {
        CreateEnumProp(prop, annoName, annoRecordField);
    } else {
        UNREACHABLE();
    }

    return annoRecordField;
}

void Emitter::AddAnnotationRecord(const std::string &annoName, const ir::ClassDeclaration *classDecl)
{
    pandasm::Record record(annoName, source_lang_);
    record.metadata->SetAccessFlags(panda::ACC_ANNOTATION);

    for (auto bodyItem : classDecl->Definition()->Body()) {
        record.field_list.emplace_back(CreateAnnotationProp(bodyItem->AsClassProperty(), annoName));
    }

    prog_->record_table.emplace(annoName, std::move(record));
}

void Emitter::AddSourceTextModuleRecord(ModuleRecordEmitter *module, CompilerContext *context)
{
    std::lock_guard<std::mutex> lock(m_);

    if (module->NeedEmitPhaseRecord()) {
        AddModuleRequestPhaseRecord(module, context);
    }

    auto moduleLiteral = std::string(context->Binder()->Program()->RecordName()) + "_" +
         std::to_string(module->Index());
    if (context->IsMergeAbc()) {
        auto moduleIdxField = panda::pandasm::Field(source_lang_);
        moduleIdxField.name = "moduleRecordIdx";
        moduleIdxField.type = panda::pandasm::Type("u32", 0);
        moduleIdxField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
                static_cast<std::string_view>(moduleLiteral)));
        rec_->field_list.emplace_back(std::move(moduleIdxField));

        if (context->PatchFixHelper()) {
            context->PatchFixHelper()->ProcessModule(rec_->name, module->Buffer());
        }
    } else {
        auto ecmaModuleRecord = panda::pandasm::Record("_ESModuleRecord",
                                                       source_lang_);
        ecmaModuleRecord.metadata->SetAccessFlags(panda::ACC_PUBLIC);

        auto moduleIdxField = panda::pandasm::Field(source_lang_);
        moduleIdxField.name = context->Binder()->Program()->ModuleRecordFieldName().empty() ?
                              std::string {context->Binder()->Program()->SourceFile()} :
                              context->Binder()->Program()->ModuleRecordFieldName();
        moduleIdxField.type = panda::pandasm::Type("u32", 0);
        moduleIdxField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
                static_cast<std::string_view>(moduleLiteral)));
        ecmaModuleRecord.field_list.emplace_back(std::move(moduleIdxField));

        if (context->PatchFixHelper()) {
            context->PatchFixHelper()->ProcessModule(ecmaModuleRecord.name, module->Buffer());
        }
        prog_->record_table.emplace(ecmaModuleRecord.name, std::move(ecmaModuleRecord));
    }
    auto &moduleLiteralsBuffer = module->Buffer();
    auto literalArrayInstance = panda::pandasm::LiteralArray(std::move(moduleLiteralsBuffer));
    prog_->literalarray_table.emplace(static_cast<std::string_view>(moduleLiteral), std::move(literalArrayInstance));
    constant_local_export_slots_ = module->GetConstantLocalExportSlots();
}

void Emitter::AddModuleRequestPhaseRecord(ModuleRecordEmitter *module, CompilerContext *context)
{
    auto phaseLiteral = std::string(context->Binder()->Program()->RecordName()) + "_" +
         std::to_string(module->PhaseIndex());
    if (context->IsMergeAbc()) {
        auto phaseIdxField = panda::pandasm::Field(source_lang_);
        phaseIdxField.name = "moduleRequestPhaseIdx";
        phaseIdxField.type = panda::pandasm::Type("u32", 0);
        phaseIdxField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
            static_cast<std::string_view>(phaseLiteral)));
        rec_->field_list.emplace_back(std::move(phaseIdxField));
    } else {
        auto moduleRequestPhaseRecord = panda::pandasm::Record("_ModuleRequestPhaseRecord",
                                                               source_lang_);
        moduleRequestPhaseRecord.metadata->SetAccessFlags(panda::ACC_PUBLIC);

        auto phaseIdxField = panda::pandasm::Field(source_lang_);
        phaseIdxField.name = "moduleRequestPhaseIdx";
        phaseIdxField.type = panda::pandasm::Type("u32", 0);
        phaseIdxField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::LITERALARRAY>(
            static_cast<std::string_view>(phaseLiteral)));
        moduleRequestPhaseRecord.field_list.emplace_back(std::move(phaseIdxField));

        prog_->record_table.emplace(moduleRequestPhaseRecord.name, std::move(moduleRequestPhaseRecord));
    }
    auto &moduleRequestPhaseLiteralsBuffer = module->PhaseBuffer();
    auto literalArrayInstance = panda::pandasm::LiteralArray(std::move(moduleRequestPhaseLiteralsBuffer));
    prog_->literalarray_table.emplace(static_cast<std::string_view>(phaseLiteral), std::move(literalArrayInstance));
}

void Emitter::AddHasTopLevelAwaitRecord(bool hasTLA, const CompilerContext *context)
{
    if (context->IsMergeAbc()) {
        auto hasTLAField = panda::pandasm::Field(source_lang_);
        hasTLAField.name = "hasTopLevelAwait";
        hasTLAField.type = panda::pandasm::Type("u8", 0);
        hasTLAField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::U8>(static_cast<uint8_t>(hasTLA)));
        rec_->field_list.emplace_back(std::move(hasTLAField));
    } else if (hasTLA) {
        auto hasTLARecord = panda::pandasm::Record("_HasTopLevelAwait", source_lang_);
        hasTLARecord.metadata->SetAccessFlags(panda::ACC_PUBLIC);
        auto hasTLAField = panda::pandasm::Field(source_lang_);
        hasTLAField.name = "hasTopLevelAwait";
        hasTLAField.type = panda::pandasm::Type("u8", 0);
        hasTLAField.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::U8>(static_cast<uint8_t>(true)));
        hasTLARecord.field_list.emplace_back(std::move(hasTLAField));

        prog_->record_table.emplace(hasTLARecord.name, std::move(hasTLARecord));
    }
}

void Emitter::AddSharedModuleRecord(const CompilerContext *context)
{
    bool isShared = context->Binder()->Program()->IsShared();

    auto sharedModuleField = panda::pandasm::Field(source_lang_);
    sharedModuleField.name = "isSharedModule";
    sharedModuleField.type = panda::pandasm::Type("u8", 0);
    sharedModuleField.metadata->SetValue(
        panda::pandasm::ScalarValue::Create<panda::pandasm::Value::Type::U8>(static_cast<uint8_t>(isShared)));

    if (context->IsMergeAbc()) {
        rec_->field_list.emplace_back(std::move(sharedModuleField));
    } else if (isShared) {
        auto sharedModuleRecord = panda::pandasm::Record("_SharedModuleRecord",
                                                         source_lang_);
        sharedModuleRecord.metadata->SetAccessFlags(panda::ACC_PUBLIC);
        sharedModuleRecord.field_list.emplace_back(std::move(sharedModuleField));
        prog_->record_table.emplace(sharedModuleRecord.name, std::move(sharedModuleRecord));
    }
}

void Emitter::AddExpectedPropertyCountRecord()
{
    static const std::string ANNOTATION = "_ESExpectedPropertyCountAnnotation";
    pandasm::Record record(ANNOTATION, pandasm::extensions::Language::ECMASCRIPT);
    record.metadata->AddAccessFlags(panda::ACC_ANNOTATION);
    prog_->record_table.emplace(ANNOTATION, std::move(record));
}

// static
void Emitter::GenBufferLiterals(ArenaVector<std::pair<int32_t, std::vector<Literal>>> &literalBuffers,
                                const LiteralBuffer *buff)
{
    auto &[idx, array] = literalBuffers.emplace_back();
    idx = buff->Index();
    constexpr size_t ARRAY_EXPANSION = 2;
    array.reserve(buff->Literals().size() * ARRAY_EXPANSION);

    for (const auto *literal : buff->Literals()) {
        panda::pandasm::LiteralArray::Literal valueLit;
        panda::pandasm::LiteralArray::Literal tagLit;

        ir::LiteralTag tag = literal->Tag();

        switch (tag) {
            case ir::LiteralTag::BOOLEAN: {
                valueLit.tag_ = panda::panda_file::LiteralTag::BOOL;
                valueLit.value_ = literal->GetBoolean();
                break;
            }
            case ir::LiteralTag::INTEGER: {
                valueLit.tag_ = panda::panda_file::LiteralTag::INTEGER;
                valueLit.value_ = literal->GetInt();
                break;
            }
            case ir::LiteralTag::DOUBLE: {
                valueLit.tag_ = panda::panda_file::LiteralTag::DOUBLE;
                valueLit.value_ = literal->GetDouble();
                break;
            }
            case ir::LiteralTag::STRING: {
                valueLit.tag_ = panda::panda_file::LiteralTag::STRING;
                valueLit.value_ = literal->GetString().Mutf8();
                break;
            }
            case ir::LiteralTag::ACCESSOR: {
                valueLit.tag_ = panda::panda_file::LiteralTag::ACCESSOR;
                valueLit.value_ = static_cast<uint8_t>(0);
                break;
            }
            case ir::LiteralTag::METHOD: {
                valueLit.tag_ = panda::panda_file::LiteralTag::METHOD;
                valueLit.value_ = literal->GetMethod().Mutf8();
                break;
            }
            case ir::LiteralTag::METHODAFFILIATE: {
                valueLit.tag_ = panda::panda_file::LiteralTag::METHODAFFILIATE;
                valueLit.value_ = literal->GetMethodAffiliate();
                break;
            }
            case ir::LiteralTag::GENERATOR_METHOD: {
                valueLit.tag_ = panda::panda_file::LiteralTag::GENERATORMETHOD;
                valueLit.value_ = literal->GetMethod().Mutf8();
                break;
            }
            case ir::LiteralTag::LITERALBUFFERINDEX: {
                valueLit.tag_ = panda::panda_file::LiteralTag::LITERALBUFFERINDEX;
                valueLit.value_ = literal->GetInt();
                break;
            }
            case ir::LiteralTag::LITERALARRAY: {
                valueLit.tag_ = panda::panda_file::LiteralTag::LITERALARRAY;
                valueLit.value_ = literal->GetString().Mutf8();
                break;
            }
            case ir::LiteralTag::BUILTINTYPEINDEX: {
                valueLit.tag_ = panda::panda_file::LiteralTag::BUILTINTYPEINDEX;
                valueLit.value_ = static_cast<uint8_t>(literal->GetInt());
                break;
            }
            case ir::LiteralTag::GETTER: {
                valueLit.tag_ = panda::panda_file::LiteralTag::GETTER;
                valueLit.value_ = literal->GetMethod().Mutf8();
                break;
            }
            case ir::LiteralTag::SETTER: {
                valueLit.tag_ = panda::panda_file::LiteralTag::SETTER;
                valueLit.value_ = literal->GetMethod().Mutf8();
                break;
            }
            case ir::LiteralTag::ASYNC_GENERATOR_METHOD: {
                valueLit.tag_ = panda::panda_file::LiteralTag::ASYNCGENERATORMETHOD;
                valueLit.value_ = literal->GetMethod().Mutf8();
                break;
            }
            case ir::LiteralTag::ETS_IMPLEMENTS: {
                valueLit.tag_ = panda::panda_file::LiteralTag::ETS_IMPLEMENTS;
                valueLit.value_ = literal->GetString().Mutf8();
                break;
            }
            case ir::LiteralTag::NULL_VALUE: {
                valueLit.tag_ = panda::panda_file::LiteralTag::NULLVALUE;
                valueLit.value_ = static_cast<uint8_t>(0);
                break;
            }
            default:
                break;
        }

        tagLit.tag_ = panda::panda_file::LiteralTag::TAGVALUE;
        tagLit.value_ = static_cast<uint8_t>(valueLit.tag_);

        array.emplace_back(tagLit);
        array.emplace_back(valueLit);
    }
}

void Emitter::DumpAsm(const panda::pandasm::Program *prog)
{
    auto &ss = std::cout;

    for (auto &[name, func] : prog->function_table) {
        ss << "slotNum = 0x" << std::hex << func.GetSlotsNum() << std::dec << std::endl;
        auto expectedCount = func.GetExpectedPropertyCount();
        if (expectedCount != 0) {
            ss << "expectedProperty = 0x" << std::hex << expectedCount << std::dec << std::endl;
        }
        ss << ".language " << func.language << std::endl;
        ss << ".function any " << name << '(';

        for (uint32_t i = 0; i < func.GetParamsNum(); i++) {
            ss << "any a" << std::to_string(i);

            if (i != func.GetParamsNum() - 1) {
                ss << ", ";
            }
        }

        ss << ") {" << std::endl;

        for (const auto &ins : func.ins) {
            ss << (ins->IsLabel() ? "" : "\t") << ins->ToString("", true, func.GetTotalRegs()) << std::endl;
        }

        ss << "}" << std::endl << std::endl;

        for (const auto &ct : func.catch_blocks) {
            ss << ".catchall " << ct.try_begin_label << ", " << ct.try_end_label << ", " << ct.catch_begin_label
               << std::endl
               << std::endl;
        }
    }

    ss << std::endl;
}

panda::pandasm::Program *Emitter::Finalize(bool dumpDebugInfo, util::PatchFix *patchFixHelper)
{
    if (dumpDebugInfo) {
        debuginfo::DebugInfoDumper dumper(prog_);
        dumper.Dump();
    }

    if (rec_) {
        delete rec_;
        rec_ = nullptr;
    }

    if (patchFixHelper) {
        patchFixHelper->Finalize(&prog_);
    }

    auto *prog = prog_;
    prog_ = nullptr;
    return prog;
}

panda::pandasm::Program *Emitter::GetProgram() const
{
    return prog_;
}

void Emitter::AddSlotNumberRecord()
{
    static const std::string SLOT_NUMBER = "_ESSlotNumberAnnotation";
    // Source files with different file type will share the same slot number record.
    // Thus the language of this record should be set as default.
    pandasm::Record record(SLOT_NUMBER, pandasm::extensions::DEFAULT_LANGUAGE);
    record.metadata->AddAccessFlags(panda::ACC_ANNOTATION);
    prog_->record_table.emplace(SLOT_NUMBER, std::move(record));
}

void Emitter::AddConcurrentModuleRequestsRecord()
{
    static const std::string CONCURRENT_MODULE_REQUESTS = "_ESConcurrentModuleRequestsAnnotation";
    // Source files with different file type will share the same slot number record.
    // Thus the language of this record should be set as default.
    pandasm::Record record(CONCURRENT_MODULE_REQUESTS, pandasm::extensions::DEFAULT_LANGUAGE);
    record.metadata->AddAccessFlags(panda::ACC_ANNOTATION);
    prog_->record_table.emplace(CONCURRENT_MODULE_REQUESTS, std::move(record));
}

}  // namespace panda::es2panda::compiler
