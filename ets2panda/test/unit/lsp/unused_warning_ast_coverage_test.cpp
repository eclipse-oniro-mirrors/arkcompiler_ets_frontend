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

#include "lsp_api_test.h"

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LSPUnusedWarningAstCoverageTests : public LSPAPITests {};

DiagnosticReferences GetSemanticDiagnostics(const std::string &fileName, const std::string &source)
{
    Initializer initializer;
    auto *context = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    EXPECT_NE(context, nullptr);
    if (context == nullptr) {
        return {};
    }

    LSPAPI const *lspApi = GetImpl();
    auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);
    return diagnostics;
}

bool HasDiagnosticMessage(const DiagnosticReferences &diagnostics, const std::string &message)
{
    return std::any_of(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                       [&message](const auto &diagnostic) { return diagnostic.message_ == message; });
}

bool HasUnusedDiagnosticData(const DiagnosticReferences &diagnostics, const std::string &message)
{
    return std::any_of(
        diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(), [&message](const auto &diagnostic) {
            if (diagnostic.message_ != message || !std::holds_alternative<std::string>(diagnostic.data_)) {
                return false;
            }
            return std::get<std::string>(diagnostic.data_) == "unusedSymbol";
        });
}

size_t CountDiagnosticMessage(const DiagnosticReferences &diagnostics, const std::string &message)
{
    return static_cast<size_t>(
        std::count_if(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                      [&message](const auto &diagnostic) { return diagnostic.message_ == message; }));
}

bool HasDiagnosticMessageAtLine(const DiagnosticReferences &diagnostics, const std::string &message, const size_t line)
{
    return std::any_of(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                       [&message, line](const auto &diagnostic) {
                           return diagnostic.message_ == message && diagnostic.range_.start.line_ == line;
                       });
}

void ExpectDiagnosticCount(const DiagnosticReferences &diagnostics, const std::string &message,
                           const size_t expectedCount)
{
    EXPECT_EQ(CountDiagnosticMessage(diagnostics, message), expectedCount) << message;
}

void ExpectDiagnostics(const DiagnosticReferences &diagnostics, const std::vector<std::string> &expectedMessages)
{
    for (const auto &message : expectedMessages) {
        EXPECT_TRUE(HasDiagnosticMessage(diagnostics, message)) << message;
        EXPECT_TRUE(HasUnusedDiagnosticData(diagnostics, message)) << message;
        ExpectDiagnosticCount(diagnostics, message, 1U);
    }
}

void ExpectNoDiagnostics(const DiagnosticReferences &diagnostics, const std::vector<std::string> &unexpectedMessages)
{
    for (const auto &message : unexpectedMessages) {
        ExpectDiagnosticCount(diagnostics, message, 0U);
    }
}

constexpr char IMPORT_COVERAGE_SOURCE[] = R"(
export default class UsedDefaultImport {
    value(): number {
        return 1.0
    }
}

export class UsedNamedImport {
    value(): number {
        return 1.0
    }
}

export let usedNamespaceValue: number = 1
)";

constexpr char IMPORT_COVERAGE_MAIN[] = R"(
import UsedDefaultImport from "./unused_warning_import_coverage_source"
import { UsedNamedImport } from "./unused_warning_import_coverage_source"
import * as UsedNamespace from "./unused_warning_import_coverage_source"
import * as UnusedNamespace from "./unused_warning_import_coverage_source"

function useImports(): number {
    let defaultValue = new UsedDefaultImport()
    let namedValue = new UsedNamedImport()
    return defaultValue.value() + namedValue.value() + UsedNamespace.usedNamespaceValue
}

useImports()
)";

constexpr char DECLARATION_NODE_COVERAGE_SOURCE[] = R"(
class UsedBaseClass {}
interface UsedParentInterface {}
type UsedAlias = number

namespace UsedNamespace {
    export type UsedMemberAlias = number
    export let usedValue: number = 1
    let unusedValue: number = 2

    export function usedHelper(value: number): number {
        return value
    }

    function unusedHelper(value: number): number {
        return value
    }
}

class UsedChildClass extends UsedBaseClass implements UsedParentInterface {
    value(): number {
        return 1.0
    }
}

class MemberDeclarationCoverage {
    private usedField: number = 1.0
    private writtenOnlyField: number = 0.0
    private unusedField: number = 2.0

    private usedMethod(): number {
        return this.usedField
    }

    private unusedMethod(): number {
        return 1.0
    }

    run(usedParam: number, unusedParam: number, ...unusedRest: number[]): number {
        this.writtenOnlyField = 1.0
        return usedParam + this.usedMethod()
    }
}

function useAlias(value: UsedAlias): UsedNamespace.UsedMemberAlias {
    return value
}

function usedFunction(arg: number): number {
    return arg
}

function unusedFunction(): void {}
class UnusedClass {}
enum UnusedEnum {
    VALUE,
}
interface UnusedInterface {}
type UnusedAlias = number

function useDeclarations(): number {
    let child = new UsedChildClass()
    let member = new MemberDeclarationCoverage()
    return child.value() + member.run(1.0, 2.0, 3.0) + useAlias(1.0) + usedFunction(UsedNamespace.usedValue)
}

useDeclarations()
)";

constexpr char ENUM_AND_TYPE_REFERENCE_COVERAGE_SOURCE[] = R"(
enum UsedEnumByMemberAccess {
    RED,
    BLUE,
}

interface UsedInterfaceByType {
    value(): number
}

type UsedAliasByType = number

class UsedClassByType implements UsedInterfaceByType {
    value(): number {
        return 1.0
    }
}

function useTypeReferences(value: UsedAliasByType, object: UsedInterfaceByType): UsedClassByType {
    let color = UsedEnumByMemberAccess.RED
    let enumValue = color == UsedEnumByMemberAccess.RED ? 1.0 : 0.0
    console.log(value + object.value() + enumValue)
    return new UsedClassByType()
}

useTypeReferences(1.0, new UsedClassByType())
)";

constexpr char ENUM_MEMBER_ACCESS_BOUNDARY_SOURCE[] = R"(
enum UsedEnumBoundary {
    VALUE,
}

function useEnumBoundary(): number {
    let value = UsedEnumBoundary.VALUE
    return value
}

useEnumBoundary()
)";

constexpr char SHADOWED_ENUM_BOUNDARY_SOURCE[] = R"(
enum ShadowedEnumBoundary {
    VALUE,
}

function useLocalShadowedEnum(): number {
    let ShadowedEnumBoundary = 1.0
    return ShadowedEnumBoundary
}

useLocalShadowedEnum()
)";

constexpr char SAME_NAMED_SCOPED_ENUM_BOUNDARY_SOURCE[] = R"(
namespace UnusedEnumScope {
    enum SameNameEnum {
        UNUSED,
    }
}

namespace UsedEnumScope {
    enum SameNameEnum {
        USED,
    }

    export function useScopedEnum(): number {
        let value = SameNameEnum.USED
        return value
    }
}

UsedEnumScope.useScopedEnum()
)";

constexpr char SAME_NAMED_OUTER_AND_SCOPED_ENUM_BOUNDARY_SOURCE[] = R"(
enum SameOuterEnum {
    OUTER,
}

namespace UsedInnerEnumScope {
    enum SameOuterEnum {
        INNER,
    }

    export function useInnerScopedEnum(): number {
        let value = SameOuterEnum.INNER
        return value
    }
}

UsedInnerEnumScope.useInnerScopedEnum()
)";

constexpr char REFERENCE_AND_TRAVERSAL_COVERAGE_SOURCE[] = R"(
class ValueObject {
    value: number = 0.0
}

class Holder {
    field: number = 1.0

    method(value: number): number {
        return this.field + value
    }
}

function useReferenceTraversal(flag: boolean, input: number): number {
    let holder = new Holder()
    let values: number[] = [input, 2.0]
    let index = 0
    let item: number = values[index]
    let objectValue: ValueObject = { value: item }
    let total = objectValue.value + holder.method(input)

    if (flag) {
        total += input
    } else {
        total += -input
    }

    for (let i: number = 0.0; i < 2.0; i++) {
        total += i
    }

    for (let element of values) {
        total += element
    }

    while (total < 20.0) {
        total++
    }

    switch (index) {
        case 0:
            total += values[index]
            break
        default:
            total += 1.0
            break
    }

    try {
        console.log(total)
    } catch (usedError) {
        console.log(usedError)
    } finally {
        total += 1.0
    }

    let arrow = (arrowParam: number): number => arrowParam + total
    return flag ? arrow(total) : total
}

useReferenceTraversal(true, 1.0)
)";

constexpr char READ_WRITE_HIGH_RISK_COVERAGE_SOURCE[] = R"(
class AssignmentHolder {
    field: number = 0.0
}

class ShorthandObject {
    value: number = 0.0
}

function useReadWriteCases(input: number): number {
    let pureWrite = 0.0
    pureWrite = 1.0

    let compoundValue = 0.0
    compoundValue += input

    let updatedValue = 0.0
    updatedValue++

    let holder = new AssignmentHolder()
    let values: number[] = [0.0, 0.0]
    let index = 0
    let value = 1.0
    let field = 2.0

    holder.field = value
    values[index] = value

    let shorthandObject: ShorthandObject = { value }
    return compoundValue + updatedValue + holder.field + values[index] + shorthandObject.value
}

useReadWriteCases(1.0)
)";

constexpr char CATCH_PARAMETER_COVERAGE_SOURCE[] = R"(
function unusedCatchParameter(): void {
    try {
        console.log("try")
    } catch (unusedError) {
    }
}

function usedCatchParameter(): void {
    try {
        console.log("try")
    } catch (usedError) {
        console.log(usedError)
    }
}

unusedCatchParameter()
usedCatchParameter()
)";

constexpr char UNUSED_IMPORT_GAP_SOURCE[] = R"(
export default class DefaultImportGap {
    value(): number {
        return 1.0
    }
}

export class NamedImportGap {
    value(): number {
        return 1.0
    }
}
)";

constexpr char UNUSED_IMPORT_GAP_MAIN[] = R"(
import UnusedDefaultGap from "./unused_warning_import_gap_source"
import { NamedImportGap as UnusedNamedGap } from "./unused_warning_import_gap_source"

function entry(): number {
    return 1.0
}

entry()
)";

constexpr char OVERLOAD_DECLARATION_COVERAGE_SOURCE[] = R"(
namespace OverloadNamespace {
    function usedNumber(value: number): number {
        return value
    }

    function usedString(value: string): number {
        console.log(value)
        return 1.0
    }

    overload used { usedNumber, usedString }

    function unusedNumber(value: number): number {
        return value
    }

    function unusedString(value: string): number {
        console.log(value)
        return 2.0
    }

    overload unused { unusedNumber, unusedString }

    export function run(): number {
        return used(1.0)
    }
}

function entry(): number {
    return OverloadNamespace.run()
}

entry()
)";

constexpr char DEFAULT_PARAMETER_COVERAGE_SOURCE[] = R"(
function defaultParameterCoverage(usedParam: number = 1.0, unusedDefaultParam: number = 2.0): number {
    return usedParam
}

defaultParameterCoverage()
)";

constexpr char COMPLEX_TYPE_REFERENCE_COVERAGE_SOURCE[] = R"(
class UsedComplexClass {
    value: number = 1.0
}

class GenericBox<T> {
    value: T

    constructor(value: T) {
        this.value = value
    }
}

interface GenericInterface<T> {
    get(): T
}

type ComplexArrayAlias = UsedComplexClass[]
type ComplexFunctionAlias = (value: UsedComplexClass) => UsedComplexClass

class GenericInterfaceImpl implements GenericInterface<UsedComplexClass> {
    private value: UsedComplexClass = new UsedComplexClass()

    get(): UsedComplexClass {
        return this.value
    }
}

function useComplexTypes(values: ComplexArrayAlias, fn: ComplexFunctionAlias,
                         box: GenericBox<UsedComplexClass>,
                         iface: GenericInterface<UsedComplexClass>): UsedComplexClass {
    let fromArray = values[0]
    let fromFunction = fn(fromArray)
    let fromBox = box.value
    let fromInterface = iface.get()
    console.log(fromFunction.value + fromBox.value + fromInterface.value)
    return fromArray
}

function identity(value: UsedComplexClass): UsedComplexClass {
    return value
}

useComplexTypes([new UsedComplexClass()], identity, new GenericBox<UsedComplexClass>(new UsedComplexClass()),
                new GenericInterfaceImpl())
)";

constexpr char BODYLESS_SIGNATURE_PARAMETER_BOUNDARY_SOURCE[] = R"(
interface BodylessSignatureInterface {
    run(arg: number): void
}

class BodylessSignatureImpl implements BodylessSignatureInterface {
    run(arg: number): void {
        console.log(arg)
    }
}

function useBodylessSignature(value: BodylessSignatureInterface): void {
    value.run(1.0)
}

useBodylessSignature(new BodylessSignatureImpl())
)";

constexpr char SHADOWING_COVERAGE_SOURCE[] = R"(
let shadowedValue = 1.0

class ShadowingClassA {
    private value: number = 1.0

    run(): number {
        return this.value
    }
}

class ShadowingClassB {
    private value: number = 2.0

    run(): number {
        let value = 3.0
        return this.value + value
    }
}

function useShadowing(): number {
    let shadowedValue = 2.0
    let classA = new ShadowingClassA()
    let classB = new ShadowingClassB()
    return shadowedValue + classA.run() + classB.run()
}

useShadowing()
)";

constexpr char DESTRUCTURING_COVERAGE_SOURCE[] = R"(
function getValues(): number[] {
    return [1.0, 2.0]
}

function useDestructuring(): number {
    let [value, unused] = getValues()
    return value
}

useDestructuring()
)";

constexpr char CATCH_WITHOUT_PARAMETER_COVERAGE_SOURCE[] = R"(
function catchWithoutParameter(): number {
    try {
        return 1
    } catch {
        return 2
    }
}

catchWithoutParameter()
)";

constexpr char MIXED_IMPORT_SOURCE[] = R"(
export default class MixedDefault {
    value(): number {
        return 1.0
    }
}

export class MixedUsed {
    value(): number {
        return 1.0
    }
}

export class MixedUnused {}
export class ReExported {}
)";

constexpr char MIXED_IMPORT_MAIN[] = R"(
import MixedDefault, { MixedUsed, MixedUnused as MixedUnusedAlias } from "./unused_warning_mixed_import_source"
export { ReExported } from "./unused_warning_mixed_import_source"

function run(): number {
    let defaultValue = new MixedDefault()
    let usedValue = new MixedUsed()
    return defaultValue.value() + usedValue.value()
}

run()
)";

constexpr char DESTRUCTURING_EDGE_COVERAGE_SOURCE[] = R"(
function values(): number[] {
    return [1.0, 2.0, 3.0]
}

function useDestructuringEdges(): number {
    let [used, middle, unusedHole] = values()
    let a = 0.0;
    let b = 0.0;
    [a, b] = values();
    return used + middle + a + b
}

useDestructuringEdges()
)";

constexpr char WRITE_ONLY_DESTRUCTURING_ASSIGNMENT_BOUNDARY_SOURCE[] = R"(
function writeOnlyValues(): number[] {
    return [1.0, 2.0]
}

function writeOnlyDestructuringAssignment(): number {
    let a = 0.0;
    let b = 0.0;
    [a, b] = writeOnlyValues();
    return 1.0
}

writeOnlyDestructuringAssignment()
)";

constexpr char STATIC_ACCESSOR_CONSTRUCTOR_COVERAGE_SOURCE[] = R"(
class StaticPrivateAccessorCase {
    private static usedStaticField: number = 1.0
    private static unusedStaticField: number = 2.0
    private _usedAccessorValue: number = 3.0
    private _unusedAccessorValue: number = 4.0

    private static usedStaticMethod(): number {
        return StaticPrivateAccessorCase.usedStaticField
    }

    private static unusedStaticMethod(): number {
        return 1.0
    }

    private get usedAccessor(): number {
        return this._usedAccessorValue
    }

    private get unusedAccessor(): number {
        return this._unusedAccessorValue
    }

    run(): number {
        return StaticPrivateAccessorCase.usedStaticMethod() + this.usedAccessor
    }
}

class ConstructorCase {
    private field: number = 0.0

    constructor(usedParam: number, unusedParam: number) {
        this.field = usedParam
    }

    value(): number {
        return this.field
    }
}

function useStaticAccessorConstructor(): number {
    let staticPrivate = new StaticPrivateAccessorCase()
    let constructorCase = new ConstructorCase(1.0, 2.0)
    return staticPrivate.run() + constructorCase.value()
}

useStaticAccessorConstructor()
)";

constexpr char ARROW_SHADOWING_COVERAGE_SOURCE[] = R"(
function useArrowShadowing(): number {
    let captured = 1.0
    let shadowed = 2.0
    let unusedOuter = 3.0
    let inner = (shadowed: number): number => {
        let innerUnused = 4.0
        return captured + shadowed
    }
    return inner(5.0)
}

useArrowShadowing()
)";

constexpr char NON_ALIAS_IMPORT_BOUNDARY_SOURCE[] = R"(
export class UnusedPlainNamedGap {
    value(): number {
        return 1.0
    }
}
)";

constexpr char NON_ALIAS_IMPORT_BOUNDARY_MAIN[] = R"(
import { UnusedPlainNamedGap } from "./unused_warning_non_alias_import_boundary_source"

function entry(): number {
    return 1.0
}

entry()
)";

constexpr char USE_BEFORE_DECLARATION_BOUNDARY_SOURCE[] = R"(
function entry(): number {
    return usedBeforeDeclaration()
}

function usedBeforeDeclaration(): number {
    return 1.0
}

entry()
)";

constexpr char SKIPPED_DECLARATIONS_BOUNDARY_SOURCE[] = R"(
export class ExportedBoundaryClass {}

export function exportedBoundaryFunction(): void {}

export let exportedBoundaryValue: number = 1.0

export default class DefaultExportedBoundaryClass {}

declare function declaredBoundaryFunction(): void

declare class DeclaredBoundaryClass {}

function entry(): number {
    return 1.0
}

entry()
)";

constexpr char PRIVATE_SETTER_BOUNDARY_SOURCE[] = R"(
class PrivateSetterCase {
    private _value: number = 0.0

    private set usedSetter(value: number) {
        this._value = value
    }

    private set unusedSetter(value: number) {
        this._value = value
    }

    run(): number {
        this.usedSetter = 1.0
        return this._value
    }
}

function usePrivateSetter(): number {
    let instance = new PrivateSetterCase()
    return instance.run()
}

usePrivateSetter()
)";

constexpr char STATIC_PRIVATE_SETTER_BOUNDARY_SOURCE[] = R"(
class StaticPrivateSetterCase {
    private static _value: number = 0.0

    private static set usedSetter(value: number) {
        StaticPrivateSetterCase._value = value
    }

    private static set unusedSetter(value: number) {
        StaticPrivateSetterCase._value = value
    }

    static run(): number {
        StaticPrivateSetterCase.usedSetter = 1.0
        return StaticPrivateSetterCase._value
    }
}

function useStaticPrivateSetter(): number {
    return StaticPrivateSetterCase.run()
}

useStaticPrivateSetter()
)";

constexpr char ACCESSOR_PAIR_BOUNDARY_SOURCE[] = R"(
class AccessorPairCase {
    private _value: number = 0.0
    private _unusedValue: number = 0.0

    private get value(): number {
        return this._value
    }

    private set value(value: number) {
        this._value = value
    }

    private get unusedOnly(): number {
        return this._unusedValue
    }

    private set unusedOnly(value: number) {
        this._unusedValue = value
    }

    write(): number {
        this.value = 1.0
        return this._value
    }

    read(): number {
        return this.value
    }
}

function useAccessorPair(): number {
    let instance = new AccessorPairCase()
    return instance.write() + instance.read()
}

useAccessorPair()
)";

constexpr char EXPRESSION_NODE_BOUNDARY_SOURCE[] = R"(
class ConstructedExpressionValue {
    private stored: number = 0.0

    constructor(value: number) {
        this.stored = value
    }

    value(): number {
        return this.stored
    }
}

class ObjectLiteralValue {
    value: number = 0.0
}

function callExpressionTarget(value: number): number {
    return value
}

function useExpressionNodeBoundaries(flag: boolean, input: number): number {
    let callArg = input
    let constructedArg = input + 1.0
    let instance = new ConstructedExpressionValue(constructedArg)
    let arrayValue = [callArg][0]
    let objectValue: ObjectLiteralValue = { value: arrayValue }
    let unaryValue = -objectValue.value
    let binaryValue = unaryValue + instance.value()
    let conditionalValue = flag ? binaryValue : input
    return callExpressionTarget(conditionalValue)
}

useExpressionNodeBoundaries(true, 1.0)
)";

constexpr char COMPUTED_MEMBER_ASSIGNMENT_BOUNDARY_SOURCE[] = R"(
class ComputedMemberOwner {
    values: number[] = [0.0, 0.0]
}

function useComputedMemberAssignment(input: number): number {
    let owner = new ComputedMemberOwner()
    let index = 0
    let value = input
    owner.values[index] = value
    return owner.values[index]
}

useComputedMemberAssignment(1.0)
)";

constexpr char NON_COMPUTED_MEMBER_PROPERTY_BOUNDARY_SOURCE[] = R"(
class NonComputedMemberOwner {
    field: number = 0.0
}

function useNonComputedMemberPropertyBoundary(): number {
    let owner = new NonComputedMemberOwner()
    let field = 1.0
    owner.field = 2.0
    return owner.field
}

useNonComputedMemberPropertyBoundary()
)";

constexpr char MEMBER_PROPERTY_DOES_NOT_REFERENCE_TOP_LEVEL_FUNCTION_SOURCE[] = R"(
function log(): void {}

function useConsoleLog(): number {
    console.log("x")
    return 1.0
}

useConsoleLog()
)";

constexpr char PRIVATE_MEMBER_ACCESS_THROUGH_SAME_CLASS_INSTANCE_SOURCE[] = R"(
class Vault {
    private secret: number = 42.0

    readFrom(other: Vault): number {
        return other.secret
    }
}

function useVault(): number {
    let first = new Vault()
    let second = new Vault()
    return first.readFrom(second)
}

useVault()
)";

constexpr char NESTED_BLOCK_DECLARATION_BOUNDARY_SOURCE[] = R"(
function useNestedBlockDeclarations(): number {
    let outerValue = 1.0
    {
        let innerUsed = 2.0
        let innerUnused = 3.0
        outerValue += innerUsed
    }
    return outerValue
}

useNestedBlockDeclarations()
)";

constexpr char REST_DESTRUCTURING_BOUNDARY_SOURCE[] = R"(
function restValues(): number[] {
    return [1.0, 2.0, 3.0]
}

function useRestDestructuringBoundary(): number {
    let [head, ...unusedTail] = restValues()
    return head
}

useRestDestructuringBoundary()
)";

constexpr char MULTI_STEP_DESTRUCTURING_BOUNDARY_SOURCE[] = R"(
function multiStepValues(): number[][] {
    return [[1.0], [2.0]]
}

function useMultiStepDestructuringBoundary(): number {
    let rows = multiStepValues()
    let [usedNested] = rows[0]
    let [unusedNested] = rows[1]
    return usedNested
}

useMultiStepDestructuringBoundary()
)";

constexpr char OBJECT_MEMBER_EXTRACTION_BOUNDARY_SOURCE[] = R"(
class ObjectMemberExtractionValue {
    first: number = 0.0
    second: number = 0.0
}

function useObjectMemberExtractionBoundary(): number {
    let record: ObjectMemberExtractionValue = { first: 1.0, second: 2.0 }
    let first = record.first
    let unusedSecond = record.second
    return first
}

useObjectMemberExtractionBoundary()
)";

constexpr char IMPORT_SHADOWING_BOUNDARY_SOURCE[] = R"(
export class ShadowedImport {
    value(): number {
        return 1.0
    }
}
)";

constexpr char IMPORT_SHADOWING_BOUNDARY_MAIN[] = R"(
import { ShadowedImport } from "./unused_warning_import_shadowing_boundary_source"

function useLocalShadow(): number {
    let ShadowedImport = 1.0
    return ShadowedImport
}

useLocalShadow()
)";

constexpr char NAMESPACE_MEMBER_ISOLATION_BOUNDARY_SOURCE[] = R"(
namespace NamespaceBoundaryA {
    let value: number = 1.0

    export function run(): number {
        return value
    }
}

namespace NamespaceBoundaryB {
    let value: number = 2.0
}

function useNamespaceBoundary(): number {
    return NamespaceBoundaryA.run()
}

useNamespaceBoundary()
)";

TEST_F(LSPUnusedWarningAstCoverageTests, CoversImportDeclarationSpecifiers)
{
    const std::vector<std::string> files = {"unused_warning_import_coverage_source.ets",
                                            "unused_warning_import_coverage_main.ets"};
    const std::vector<std::string> contents = {IMPORT_COVERAGE_SOURCE, IMPORT_COVERAGE_MAIN};
    auto paths = CreateTempFile(files, contents);
    ASSERT_EQ(paths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED, contents[1].c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ExpectDiagnostics(diagnostics, {
                                       "'UnusedNamespace' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'UsedDefaultImport' is never used",
                                         "'UsedNamedImport' is never used",
                                         "'UsedNamespace' is never used",
                                         "'useImports' is never used",
                                         "'defaultValue' is never used",
                                         "'namedValue' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversDeclarationNodes)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_declaration_node_coverage.ets", DECLARATION_NODE_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedValue' is never used",
                                       "'unusedHelper' is never used",
                                       "'writtenOnlyField' is never used",
                                       "'unusedField' is never used",
                                       "'unusedMethod' is never used",
                                       "'unusedParam' is never used",
                                       "'unusedRest' is never used",
                                       "'unusedFunction' is never used",
                                       "'UnusedClass' is never used",
                                       "'UnusedEnum' is never used",
                                       "'UnusedInterface' is never used",
                                       "'UnusedAlias' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'UsedBaseClass' is never used",
                                         "'UsedParentInterface' is never used",
                                         "'UsedAlias' is never used",
                                         "'UsedNamespace' is never used",
                                         "'UsedMemberAlias' is never used",
                                         "'usedValue' is never used",
                                         "'usedHelper' is never used",
                                         "'UsedChildClass' is never used",
                                         "'MemberDeclarationCoverage' is never used",
                                         "'usedField' is never used",
                                         "'usedMethod' is never used",
                                         "'usedParam' is never used",
                                         "'useAlias' is never used",
                                         "'usedFunction' is never used",
                                         "'useDeclarations' is never used",
                                         "'child' is never used",
                                         "'member' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversEnumAndTypeReferences)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_enum_type_reference_coverage.ets",
                                                    ENUM_AND_TYPE_REFERENCE_COVERAGE_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'UsedEnumByMemberAccess' is never used",
                                         "'UsedInterfaceByType' is never used",
                                         "'UsedAliasByType' is never used",
                                         "'UsedClassByType' is never used",
                                         "'useTypeReferences' is never used",
                                         "'value' is never used",
                                         "'object' is never used",
                                         "'color' is never used",
                                         "'enumValue' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversEnumMemberAccessBoundary)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_enum_member_access_boundary.ets", ENUM_MEMBER_ACCESS_BOUNDARY_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'UsedEnumBoundary' is never used",
                                         "'useEnumBoundary' is never used",
                                         "'value' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversShadowedEnumBoundary)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_shadowed_enum_boundary.ets", SHADOWED_ENUM_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'ShadowedEnumBoundary' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'useLocalShadowedEnum' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversSameNamedScopedEnumBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_same_named_scoped_enum_boundary.ets",
                                                    SAME_NAMED_SCOPED_ENUM_BOUNDARY_SOURCE);

    ExpectDiagnosticCount(diagnostics, "'SameNameEnum' is never used", 1U);
    EXPECT_TRUE(HasDiagnosticMessageAtLine(diagnostics, "'SameNameEnum' is never used", 3U));
    EXPECT_FALSE(HasDiagnosticMessageAtLine(diagnostics, "'SameNameEnum' is never used", 9U));
    ExpectNoDiagnostics(diagnostics, {
                                         "'useScopedEnum' is never used",
                                         "'value' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversSameNamedOuterAndScopedEnumBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_same_named_outer_and_scoped_enum_boundary.ets",
                                                    SAME_NAMED_OUTER_AND_SCOPED_ENUM_BOUNDARY_SOURCE);

    ExpectDiagnosticCount(diagnostics, "'SameOuterEnum' is never used", 1U);
    EXPECT_TRUE(HasDiagnosticMessageAtLine(diagnostics, "'SameOuterEnum' is never used", 2U));
    EXPECT_FALSE(HasDiagnosticMessageAtLine(diagnostics, "'SameOuterEnum' is never used", 8U));
    ExpectNoDiagnostics(diagnostics, {
                                         "'useInnerScopedEnum' is never used",
                                         "'value' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversExpressionReferenceAndContainerTraversalNodes)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_reference_traversal_coverage.ets",
                                                    REFERENCE_AND_TRAVERSAL_COVERAGE_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'ValueObject' is never used",
                                         "'Holder' is never used",
                                         "'field' is never used",
                                         "'method' is never used",
                                         "'useReferenceTraversal' is never used",
                                         "'flag' is never used",
                                         "'input' is never used",
                                         "'holder' is never used",
                                         "'values' is never used",
                                         "'index' is never used",
                                         "'item' is never used",
                                         "'objectValue' is never used",
                                         "'total' is never used",
                                         "'element' is never used",
                                         "'usedError' is never used",
                                         "'arrow' is never used",
                                         "'arrowParam' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversReadWriteAndHighRiskExpressionCases)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_read_write_high_risk_coverage.ets",
                                                    READ_WRITE_HIGH_RISK_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'pureWrite' is never used",
                                       "'field' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'AssignmentHolder' is never used",
                                         "'ShorthandObject' is never used",
                                         "'useReadWriteCases' is never used",
                                         "'input' is never used",
                                         "'compoundValue' is never used",
                                         "'updatedValue' is never used",
                                         "'holder' is never used",
                                         "'values' is never used",
                                         "'index' is never used",
                                         "'value' is never used",
                                         "'shorthandObject' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversCatchClauseParameters)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_catch_parameter_coverage.ets", CATCH_PARAMETER_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedError' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'unusedCatchParameter' is never used",
                                         "'usedCatchParameter' is never used",
                                         "'usedError' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversUnusedImportSpecifiersDesiredBehavior)
{
    const std::vector<std::string> files = {"unused_warning_import_gap_source.ets",
                                            "unused_warning_import_gap_main.ets"};
    const std::vector<std::string> contents = {UNUSED_IMPORT_GAP_SOURCE, UNUSED_IMPORT_GAP_MAIN};
    auto paths = CreateTempFile(files, contents);
    ASSERT_EQ(paths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED, contents[1].c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ExpectDiagnostics(diagnostics, {
                                       "'UnusedDefaultGap' is never used",
                                       "'UnusedNamedGap' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'entry' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversOverloadDeclarationsDesiredBehavior)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_overload_declaration_coverage.ets",
                                                    OVERLOAD_DECLARATION_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unused' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'OverloadNamespace' is never used",
                                         "'used' is never used",
                                         "'usedNumber' is never used",
                                         "'usedString' is never used",
                                         "'entry' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversDefaultParameterInitializersDesiredBehavior)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_default_parameter_coverage.ets", DEFAULT_PARAMETER_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedDefaultParam' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'defaultParameterCoverage' is never used",
                                         "'usedParam' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversComplexTypeReferencePositionsDesiredBehavior)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_complex_type_reference_coverage.ets",
                                                    COMPLEX_TYPE_REFERENCE_COVERAGE_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'UsedComplexClass' is never used",
                                         "'GenericBox' is never used",
                                         "'GenericInterface' is never used",
                                         "'ComplexArrayAlias' is never used",
                                         "'ComplexFunctionAlias' is never used",
                                         "'GenericInterfaceImpl' is never used",
                                         "'value' is never used",
                                         "'useComplexTypes' is never used",
                                         "'values' is never used",
                                         "'fn' is never used",
                                         "'box' is never used",
                                         "'iface' is never used",
                                         "'fromArray' is never used",
                                         "'fromFunction' is never used",
                                         "'fromBox' is never used",
                                         "'fromInterface' is never used",
                                         "'identity' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversBodylessSignatureParametersBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_bodyless_signature_parameter_boundary.ets",
                                                    BODYLESS_SIGNATURE_PARAMETER_BOUNDARY_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'BodylessSignatureInterface' is never used",
                                         "'BodylessSignatureImpl' is never used",
                                         "'run' is never used",
                                         "'arg' is never used",
                                         "'useBodylessSignature' is never used",
                                         "'value' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversShadowingDesiredBehavior)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_shadowing_coverage.ets", SHADOWING_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'shadowedValue' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'ShadowingClassA' is never used",
                                         "'ShadowingClassB' is never used",
                                         "'value' is never used",
                                         "'useShadowing' is never used",
                                         "'classA' is never used",
                                         "'classB' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversDestructuringDeclarationsDesiredBehavior)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_destructuring_coverage.ets", DESTRUCTURING_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unused' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'getValues' is never used",
                                         "'useDestructuring' is never used",
                                         "'value' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversCatchWithoutParameterDesiredBehavior)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_catch_without_parameter_coverage.ets",
                                                    CATCH_WITHOUT_PARAMETER_COVERAGE_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'catchWithoutParameter' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversMixedImportAndReExport)
{
    const std::vector<std::string> files = {"unused_warning_mixed_import_source.ets",
                                            "unused_warning_mixed_import_main.ets"};
    const std::vector<std::string> contents = {MIXED_IMPORT_SOURCE, MIXED_IMPORT_MAIN};
    auto paths = CreateTempFile(files, contents);
    ASSERT_EQ(paths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED, contents[1].c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ExpectDiagnostics(diagnostics, {
                                       "'MixedUnusedAlias' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'MixedDefault' is never used",
                                         "'MixedUsed' is never used",
                                         "'ReExported' is never used",
                                         "'run' is never used",
                                         "'defaultValue' is never used",
                                         "'usedValue' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversDestructuringElementsAndAssignments)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_destructuring_edge_coverage.ets", DESTRUCTURING_EDGE_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedHole' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'values' is never used",
                                         "'useDestructuringEdges' is never used",
                                         "'used' is never used",
                                         "'middle' is never used",
                                         "'a' is never used",
                                         "'b' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversWriteOnlyDestructuringAssignmentBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_write_only_destructuring_assignment_boundary.ets",
                                                    WRITE_ONLY_DESTRUCTURING_ASSIGNMENT_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'a' is never used",
                                       "'b' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'writeOnlyValues' is never used",
                                         "'writeOnlyDestructuringAssignment' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversStaticPrivateAccessorsAndConstructors)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_static_accessor_constructor_coverage.ets",
                                                    STATIC_ACCESSOR_CONSTRUCTOR_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedStaticField' is never used",
                                       "'unusedStaticMethod' is never used",
                                       "'unusedAccessor' is never used",
                                       "'unusedParam' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'StaticPrivateAccessorCase' is never used",
                                         "'usedStaticField' is never used",
                                         "'_usedAccessorValue' is never used",
                                         "'_unusedAccessorValue' is never used",
                                         "'usedStaticMethod' is never used",
                                         "'usedAccessor' is never used",
                                         "'ConstructorCase' is never used",
                                         "'field' is never used",
                                         "'usedParam' is never used",
                                         "'useStaticAccessorConstructor' is never used",
                                         "'staticPrivate' is never used",
                                         "'constructorCase' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversArrowScopeCaptureAndShadowing)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_arrow_shadowing_coverage.ets", ARROW_SHADOWING_COVERAGE_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'shadowed' is never used",
                                       "'unusedOuter' is never used",
                                       "'innerUnused' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'useArrowShadowing' is never used",
                                         "'captured' is never used",
                                         "'inner' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversNonAliasUnusedNamedImportBoundary)
{
    const std::vector<std::string> files = {"unused_warning_non_alias_import_boundary_source.ets",
                                            "unused_warning_non_alias_import_boundary_main.ets"};
    const std::vector<std::string> contents = {NON_ALIAS_IMPORT_BOUNDARY_SOURCE, NON_ALIAS_IMPORT_BOUNDARY_MAIN};
    auto paths = CreateTempFile(files, contents);
    ASSERT_EQ(paths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED, contents[1].c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ExpectDiagnostics(diagnostics, {
                                       "'UnusedPlainNamedGap' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'entry' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversUseBeforeFunctionDeclarationBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_use_before_declaration_boundary.ets",
                                                    USE_BEFORE_DECLARATION_BOUNDARY_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'entry' is never used",
                                         "'usedBeforeDeclaration' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversSkippedExportAndDeclareDeclarationsBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_skipped_declarations_boundary.ets",
                                                    SKIPPED_DECLARATIONS_BOUNDARY_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'ExportedBoundaryClass' is never used",
                                         "'exportedBoundaryFunction' is never used",
                                         "'exportedBoundaryValue' is never used",
                                         "'DefaultExportedBoundaryClass' is never used",
                                         "'declaredBoundaryFunction' is never used",
                                         "'DeclaredBoundaryClass' is never used",
                                         "'entry' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversPrivateSetterBoundary)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_private_setter_boundary.ets", PRIVATE_SETTER_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedSetter' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'PrivateSetterCase' is never used",
                                         "'_value' is never used",
                                         "'usedSetter' is never used",
                                         "'usePrivateSetter' is never used",
                                         "'instance' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversStaticPrivateSetterBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_static_private_setter_boundary.ets",
                                                    STATIC_PRIVATE_SETTER_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedSetter' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'StaticPrivateSetterCase' is never used",
                                         "'_value' is never used",
                                         "'usedSetter' is never used",
                                         "'useStaticPrivateSetter' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversAccessorPairWithSameNameBoundary)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_accessor_pair_boundary.ets", ACCESSOR_PAIR_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedOnly' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'AccessorPairCase' is never used",
                                         "'_value' is never used",
                                         "'_unusedValue' is never used",
                                         "'value' is never used",
                                         "'write' is never used",
                                         "'read' is never used",
                                         "'useAccessorPair' is never used",
                                         "'instance' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversImportShadowedByLocalBindingBoundary)
{
    const std::vector<std::string> files = {"unused_warning_import_shadowing_boundary_source.ets",
                                            "unused_warning_import_shadowing_boundary_main.ets"};
    const std::vector<std::string> contents = {IMPORT_SHADOWING_BOUNDARY_SOURCE, IMPORT_SHADOWING_BOUNDARY_MAIN};
    auto paths = CreateTempFile(files, contents);
    ASSERT_EQ(paths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED, contents[1].c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ExpectDiagnostics(diagnostics, {
                                       "'ShadowedImport' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'useLocalShadow' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversNamespaceMemberNameIsolationBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_namespace_member_isolation_boundary.ets",
                                                    NAMESPACE_MEMBER_ISOLATION_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'NamespaceBoundaryB' is never used",
                                       "'value' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'NamespaceBoundaryA' is never used",
                                         "'run' is never used",
                                         "'useNamespaceBoundary' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversExpressionNodeBoundaries)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_expression_node_boundary.ets", EXPRESSION_NODE_BOUNDARY_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'ConstructedExpressionValue' is never used",
                                         "'stored' is never used",
                                         "'ObjectLiteralValue' is never used",
                                         "'value' is never used",
                                         "'callExpressionTarget' is never used",
                                         "'useExpressionNodeBoundaries' is never used",
                                         "'flag' is never used",
                                         "'input' is never used",
                                         "'callArg' is never used",
                                         "'constructedArg' is never used",
                                         "'instance' is never used",
                                         "'arrayValue' is never used",
                                         "'objectValue' is never used",
                                         "'unaryValue' is never used",
                                         "'binaryValue' is never used",
                                         "'conditionalValue' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversComputedMemberAssignmentBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_computed_member_assignment_boundary.ets",
                                                    COMPUTED_MEMBER_ASSIGNMENT_BOUNDARY_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'ComputedMemberOwner' is never used",
                                         "'values' is never used",
                                         "'useComputedMemberAssignment' is never used",
                                         "'input' is never used",
                                         "'owner' is never used",
                                         "'index' is never used",
                                         "'value' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversNonComputedMemberPropertyDoesNotReferenceLocalName)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_non_computed_member_property_boundary.ets",
                                                    NON_COMPUTED_MEMBER_PROPERTY_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'field' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'NonComputedMemberOwner' is never used",
                                         "'useNonComputedMemberPropertyBoundary' is never used",
                                         "'owner' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversMemberPropertyDoesNotReferenceTopLevelFunctionBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_member_property_top_level_function_boundary.ets",
                                                    MEMBER_PROPERTY_DOES_NOT_REFERENCE_TOP_LEVEL_FUNCTION_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'log' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'useConsoleLog' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversPrivateMemberAccessThroughSameClassInstanceBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_private_member_instance_access_boundary.ets",
                                                    PRIVATE_MEMBER_ACCESS_THROUGH_SAME_CLASS_INSTANCE_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'Vault' is never used",
                                         "'secret' is never used",
                                         "'readFrom' is never used",
                                         "'useVault' is never used",
                                         "'first' is never used",
                                         "'second' is never used",
                                         "'other' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversNestedBlockDeclarationBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_nested_block_declaration_boundary.ets",
                                                    NESTED_BLOCK_DECLARATION_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'innerUnused' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'useNestedBlockDeclarations' is never used",
                                         "'outerValue' is never used",
                                         "'innerUsed' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversRestDestructuringBoundary)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_warning_rest_destructuring_boundary.ets", REST_DESTRUCTURING_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedTail' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'restValues' is never used",
                                         "'useRestDestructuringBoundary' is never used",
                                         "'head' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversMultiStepDestructuringBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_multi_step_destructuring_boundary.ets",
                                                    MULTI_STEP_DESTRUCTURING_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedNested' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'multiStepValues' is never used",
                                         "'useMultiStepDestructuringBoundary' is never used",
                                         "'rows' is never used",
                                         "'usedNested' is never used",
                                     });
}

TEST_F(LSPUnusedWarningAstCoverageTests, CoversObjectMemberExtractionBoundary)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_warning_object_member_extraction_boundary.ets",
                                                    OBJECT_MEMBER_EXTRACTION_BOUNDARY_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedSecond' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'ObjectMemberExtractionValue' is never used",
                                         "'first' is never used",
                                         "'second' is never used",
                                         "'useObjectMemberExtractionBoundary' is never used",
                                         "'record' is never used",
                                     });
}

}  // namespace
