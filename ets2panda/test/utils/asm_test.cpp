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
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <string>

#include "asm_test.h"
#include "assembly-field.h"
#include "assembly-literals.h"
#include "assembly-program.h"

#include "generated/signatures.h"
#include "gmock/gmock.h"
#include "libarkfile/literal_data_accessor.h"

// Value printers for tests
namespace ark::pandasm {

namespace {

template <class... Ts>
struct LiteralOverloaded : Ts... {  // NOLINT (fuchsia-multiple-inheritance)
    using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template <class... Ts>
LiteralOverloaded(Ts...) -> LiteralOverloaded<Ts...>;

}  // namespace

[[maybe_unused]] static std::ostream &operator<<(std::ostream &s, decltype(LiteralArray::Literal::value) const &value)
{
    std::visit(LiteralOverloaded {
                   [&s](bool v) { s << (v ? "true" : "false"); },
                   [&s](uint8_t v) { s << std::to_string(v) << "_U8"; },
                   [&s](uint16_t v) { s << std::to_string(v) << "_U16"; },
                   [&s](uint32_t v) { s << std::to_string(v) << "_U32"; },
                   [&s](uint64_t v) { s << std::to_string(v) << "_U64"; },
                   [&s](float v) { s << std::fixed << v << std::defaultfloat << "f"; },
                   [&s](double v) { s << std::fixed << v << std::defaultfloat; },
                   [&s](const std::string &v) { s << std::quoted(v); },
               },
               value);
    return s;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &s, panda_file::LiteralTag const &value)
{
    return s << "0x" << std::hex << uint64_t(value) << std::dec << "_LT";
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &s, LiteralArray::Literal const &value)
{
    return s << "{" << value.tag << ", " << value.value << "}";
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &s, LiteralArray const &value)
{
    s << "LA({ ";
    auto it = value.literals.begin();
    if (it != value.literals.end()) {
        s << *it++;
    }
    while (it != value.literals.end()) {
        s << ", " << *it++;
    }
    s << " })";
    return s;
}

bool operator==(const LiteralArray &lhs, const LiteralArray &rhs)
{
    return lhs.literals == rhs.literals;
}
bool operator==(const LiteralArray::Literal &lhs, const LiteralArray::Literal &rhs)
{
    return lhs.tag == rhs.tag && lhs.value == rhs.value;
}

using ArrType = Program::LiteralArrayTableT;

[[maybe_unused]] static std::ostream &operator<<(std::ostream &s, ArrType::value_type const &value)
{
    return s << "{" << std::quoted(value.first) << ", " << value.second << "}";
}

[[maybe_unused]] static void PrintTo(const ArrType &value, std::ostream *s)
{
    *s << "{" << std::endl;
    for (auto const &p : value) {
        *s << "    " << p << "," << std::endl;
    }
    *s << "}" << std::endl;
}

}  // namespace ark::pandasm

namespace test::utils {

AsmTest::AsmTest()
{
    ark::mem::MemConfig::Initialize(0, 0, ark::es2panda::COMPILER_SIZE, 0, 0, 0);
    ark::PoolManager::Initialize(ark::PoolType::MMAP);
}
AsmTest::~AsmTest()
{
    ark::PoolManager::Finalize();
    ark::mem::MemConfig::Finalize();
}

ark::pandasm::Function *AsmTest::GetFunction(std::string_view functionName,
                                             const std::map<std::string, ark::pandasm::Function> &table)
{
    auto it = table.find(functionName.data());
    if (it == table.end()) {
        return nullptr;
    }
    return const_cast<ark::pandasm::Function *>(&it->second);
}

ark::pandasm::Record *AsmTest::GetRecord(std::string_view recordName,
                                         const std::unique_ptr<ark::pandasm::Program> &program)
{
    auto it = program->recordTable.find(recordName.data());
    if (it == program->recordTable.end()) {
        return nullptr;
    }
    return &it->second;
}

void AsmTest::CompareActualWithExpected(const std::string &expectedValue, ark::pandasm::ScalarValue *scalarValue,
                                        const std::string &field)
{
    std::string actualValue = std::visit(
        [](const auto &value) -> std::string {
            using ValueType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<ValueType, std::string>) {
                return value;
            } else if constexpr (std::is_arithmetic_v<ValueType>) {
                return std::to_string(value);
            } else {
                return "Unsupported type";
            }
        },
        scalarValue->GetValue());

    ASSERT_EQ(actualValue, expectedValue) << "Value mismatch for " + field;
}

void AsmTest::CheckAnnoDecl(ark::pandasm::Program *program, const std::string &annoName,
                            const std::vector<std::pair<std::string, std::string>> &expectedAnnotations)
{
    const auto &recordTable = program->recordTable;
    ASSERT_FALSE(recordTable.empty()) << "No records found in the program.";
    auto found = recordTable.find(annoName);
    ASSERT_NE(found, recordTable.end());

    for (size_t i = 0; i < expectedAnnotations.size(); i++) {
        ASSERT_EQ(expectedAnnotations[i].first, found->second.fieldList[i].name)
            << "missing expected annotation name: " << found->second.fieldList[i].name;
        ;
        auto scalarValue = found->second.fieldList[i].metadata->GetValue();
        if (scalarValue) {
            CompareActualWithExpected(expectedAnnotations[i].second, &*scalarValue, found->second.fieldList[i].name);
        }
    }
}

void AsmTest::ExpectLiteralArrayTable(
    ark::pandasm::Program *program, const ::ark::pandasm::Program::LiteralArrayTableT &expectedLiteralArrayTable) const
{
    EXPECT_THAT(program->literalarrayTable, ::testing::ContainerEq(expectedLiteralArrayTable));
}

void AsmTest::CheckLiteralArrayTable(ark::pandasm::Program *program,
                                     const AsmTest::ExpectedLiteralArrayTable &expectedLiteralArrayTable)
{
    const auto &literalarrayTable = program->literalarrayTable;
    ASSERT_FALSE(literalarrayTable.empty()) << "literalarrayTable is empty!";
    for (const auto &literalArray : expectedLiteralArrayTable) {
        // ASSERT_THAT(literalarrayTable, ::testing::Contains(::testing::Key(literalArray.first)));
        auto found = literalarrayTable.find(literalArray.first);
        ASSERT_NE(found, literalarrayTable.end()) << "Not found " << std::quoted(literalArray.first) << std::endl
                                                  << "Actual: " << ::testing::PrintToString(literalarrayTable);
        size_t i = 1;
        for (const auto &value : literalArray.second) {
            constexpr int STRIDE = 2;
            ASSERT_EQ(value, found->second.literals[i].value) << "Value mismatch for " + literalArray.first;
            i += STRIDE;
        }
    }
}

void AsmTest::CheckAnnotation(const std::vector<std::pair<std::string, std::string>> &expectedValues,
                              const ark::pandasm::AnnotationData &annotation)
{
    for (const auto &element : annotation.GetElements()) {
        auto it = std::find_if(expectedValues.begin(), expectedValues.end(),
                               [&element](const auto &pair) { return pair.first == element.GetName(); });
        if (it != expectedValues.end()) {
            CompareActualWithExpected(it->second, element.GetValue()->GetAsScalar(), element.GetName());
        } else {
            ASSERT_TRUE(false) << "missing expected annotation name: " << element.GetName();
        }
    }
}

void AsmTest::CheckRecordAnnotations(ark::pandasm::Program *program, const std::string &recordName,
                                     const AnnotationMap &expectedAnnotations)
{
    const auto &recordTable = program->recordTable;
    ASSERT_FALSE(recordTable.empty()) << "No records found in the program.";
    auto found = recordTable.find(recordName);
    ASSERT_NE(found, recordTable.end());

    for (const auto &expected : expectedAnnotations) {
        auto annotations = found->second.metadata->GetAnnotations();
        auto it = std::find_if(annotations.begin(), annotations.end(),
                               [&expected](const ark::pandasm::AnnotationData &annotation) {
                                   return annotation.GetName() == expected.first;
                               });

        ASSERT_NE(it, annotations.end()) << recordName << " missing expected annotation: " << expected.first;

        // Check the fields for the matched annotation name
        CheckAnnotation(expected.second, *it);
    }
}

void AsmTest::CheckModuleAnnotation(ark::pandasm::Program *program, const std::string &recordName, bool isModule,
                                    const std::vector<std::string> &expectedAnnotations)
{
    const auto &recordTable = program->recordTable;
    ASSERT_FALSE(recordTable.empty()) << "No records found in the program.";
    auto found = recordTable.find(recordName);
    ASSERT_NE(found, recordTable.end());

    auto annotations = found->second.metadata->GetAnnotations();
    auto it = std::find_if(annotations.begin(), annotations.end(), [](const ark::pandasm::AnnotationData &annotation) {
        return annotation.GetName() == std::string {ark::es2panda::compiler::Signatures::ETS_ANNOTATION_MODULE};
    });
    if (isModule) {
        ASSERT_NE(it, annotations.end()) << recordName << " missing expected annotation: "
                                         << ark::es2panda::compiler::Signatures::ETS_ANNOTATION_MODULE;
    } else {
        ASSERT_EQ(it, annotations.end()) << recordName << " has annotation: "
                                         << ark::es2panda::compiler::Signatures::ETS_ANNOTATION_MODULE
                                         << ", but shouldn't";
        return;
    }
    ASSERT_EQ(it->GetElements().size(), 1);
    const auto &element = it->GetElements()[0];
    ASSERT_EQ(element.GetName(), std::string {ark::es2panda::compiler::Signatures::ANNOTATION_KEY_EXPORTED})
        << recordName << "module annotation missing element "
        << ark::es2panda::compiler::Signatures::ANNOTATION_KEY_EXPORTED;

    for (const auto &val : element.GetValue()->GetAsArray()->GetValues()) {
        auto name = val.GetValue<ark::pandasm::Type>().GetName();
        auto foundExpected = std::find(expectedAnnotations.begin(), expectedAnnotations.end(), name);
        ASSERT_NE(foundExpected, expectedAnnotations.end()) << "Value mismatch for " + name;
    }
}

void AsmTest::CheckRecordWithoutAnnotations(ark::pandasm::Program *program, const std::string &recordName,
                                            bool isModule)
{
    const auto &recordTable = program->recordTable;
    ASSERT_FALSE(recordTable.empty()) << "No records found in the program.";
    auto found = recordTable.find(recordName);
    ASSERT_NE(found, recordTable.end());
    if (isModule) {
        ASSERT_EQ(found->second.metadata->GetAnnotations().size(), 1);
    } else {
        ASSERT(found->second.metadata->GetAnnotations().empty());
    }
}

void AsmTest::CheckFunctionAnnotations(ark::pandasm::Program *program, const std::string &functionName, bool isStatic,
                                       const AnnotationMap &expectedAnnotations)
{
    const auto &functionTable = isStatic ? program->functionStaticTable : program->functionInstanceTable;
    auto found = functionTable.find(functionName);
    ASSERT_NE(found, functionTable.end()) << "Unexpected function Name: " << functionName;

    for (const auto &expected : expectedAnnotations) {
        auto annotations = found->second.metadata->GetAnnotations();
        auto it = std::find_if(annotations.begin(), annotations.end(),
                               [&expected](const ark::pandasm::AnnotationData &annotation) {
                                   return annotation.GetName() == expected.first;
                               });

        ASSERT_NE(it, annotations.end()) << functionName << " missing expected annotation: " << expected.first;

        // Check the fields for the matched annotation name
        CheckAnnotation(expected.second, *it);
    }
}

void AsmTest::CheckFunctionWithoutAnnotations(ark::pandasm::Program *program, const std::string &functionName,
                                              bool isStatic)
{
    const auto &functionTable = isStatic ? program->functionStaticTable : program->functionInstanceTable;
    auto found = functionTable.find(functionName);
    ASSERT_NE(found, functionTable.end()) << "Unexpected function Name: " << functionName;
    ASSERT(found->second.metadata->GetAnnotations().empty());
}

void AsmTest::CheckFunctionParameterAnnotations(ark::pandasm::Program *program, const std::string &functionName,
                                                bool isStatic, const uint32_t &paramIndex,
                                                const AnnotationMap &expectedAnnotations)
{
    const auto &functionTable = isStatic ? program->functionStaticTable : program->functionInstanceTable;
    auto found = functionTable.find(functionName);
    ASSERT_NE(found, functionTable.end());
    ASSERT_LT(paramIndex, found->second.params.size());

    for (const auto &expected : expectedAnnotations) {
        auto annotations = found->second.params.at(paramIndex).GetOrCreateMetadata()->GetAnnotations();
        auto it = std::find_if(annotations.begin(), annotations.end(),
                               [&expected](const ark::pandasm::AnnotationData &annotation) {
                                   return annotation.GetName() == expected.first;
                               });

        ASSERT_NE(it, annotations.end()) << functionName << "param at " << paramIndex
                                         << " missing expected annotation: " << expected.first;

        // Check the fields for the matched annotation name
        CheckAnnotation(expected.second, *it);
    }
}

void AsmTest::CheckFunctionParameterWithoutAnnotations(ark::pandasm::Program *program, const std::string &functionName,
                                                       bool isStatic, const uint32_t &paramIndex)
{
    const auto &functionTable = isStatic ? program->functionStaticTable : program->functionInstanceTable;
    auto found = functionTable.find(functionName);
    ASSERT_NE(found, functionTable.end());
    ASSERT_LT(paramIndex, found->second.params.size());
    ASSERT(found->second.params.at(paramIndex).GetOrCreateMetadata()->GetAnnotations().empty());
}

void AsmTest::CheckClassFieldAnnotations(ark::pandasm::Program *program, const std::string &recordName,
                                         const std::string &fieldName, const AnnotationMap &expectedAnnotations)
{
    const auto &recordTable = program->recordTable;
    auto found = recordTable.find(recordName);
    ASSERT_NE(found, recordTable.end());
    auto &filedList = found->second.fieldList;
    auto result = std::find_if(filedList.begin(), filedList.end(),
                               [&fieldName](const ark::pandasm::Field &field) { return field.name == fieldName; });
    ASSERT_NE(result, filedList.end()) << "Cannot find classProperty '" << fieldName << "'.";
    for (const auto &expected : expectedAnnotations) {
        auto annotations = result->metadata->GetAnnotations();
        auto it = std::find_if(annotations.begin(), annotations.end(),
                               [&expected](const ark::pandasm::AnnotationData &annotation) {
                                   return annotation.GetName() == expected.first;
                               });

        ASSERT_NE(it, annotations.end()) << fieldName << " missing expected annotation: " << expected.first;

        // Check the fields for the matched annotation name
        CheckAnnotation(expected.second, *it);
    }
}

void AsmTest::CheckClassFieldWithoutAnnotations(ark::pandasm::Program *program, const std::string &recordName,
                                                const std::string &fieldName)
{
    const auto &recordTable = program->recordTable;
    auto found = recordTable.find(recordName);
    ASSERT_NE(found, recordTable.end());
    auto &filedList = found->second.fieldList;
    auto result = std::find_if(filedList.begin(), filedList.end(),
                               [&fieldName](const ark::pandasm::Field &field) { return field.name == fieldName; });
    ASSERT_NE(result, filedList.end()) << "Cannot find classProperty '" << fieldName << "'.";
    ASSERT(result->metadata->GetAnnotations().empty());
}

void AsmTest::SetCurrentProgram(std::string_view src)
{
    int argc = 1;
    const char *argv = "../../../../bin/es2panda";  // NOLINT(modernize-avoid-c-arrays)
    static constexpr std::string_view FILE_NAME = "dummy.ets";

    program_ = GetProgram(argc, &argv, FILE_NAME, src);
    ASSERT_NE(program_.get(), nullptr);
}

std::unique_ptr<ark::pandasm::Program> AsmTest::GetCurrentProgram(std::string_view src)
{
    static constexpr std::string_view FILE_NAME = "dummy.ets";
    std::array<char const *, 2> args = {"../../../../../bin/es2panda",
                                        "--ets-unnamed"};  // NOLINT(modernize-avoid-c-arrays)

    auto program = GetProgram(args.size(), args.data(), FILE_NAME, src);
    return program;
}

}  // namespace test::utils
