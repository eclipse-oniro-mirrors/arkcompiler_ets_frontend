/*
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

@Target([AnnotationTargets.INTERFACE_METHOD])
@Retention("SOURCE")
@interface anno{ }

// ============================================================================
// Top-Level Declarations
// ============================================================================

// NAMESPACE
@anno
namespace TestNamespace {
    @anno
    let namespaceVar: number = 1;
}

// VARIABLE
@anno
let topLevelVar: number = 1;

// TYPE_ALIAS
@anno
type TypeAlias = number;

// ENUMERATION
@anno
enum TestEnum {
    A = 1,
    B = 2
}

// FUNCTION
@anno
function testFunction(): void { }

// FUNCTION_WITH_RECEIVER (extension function)
class ReceiverClass {
    value: number = 1;
}

@anno
function testExtension(this: ReceiverClass): void { }

// CLASS
@anno
class TestClass {
    // CLASS_FIELD
    @anno
    classField: number = 1;

    // CLASS_METHOD
    @anno
    classMethod(): void { }

    // CLASS_GETTER
    @anno
    get classGetter(): number {
        return 1;
    }

    // CLASS_SETTER
    @anno
    set classSetter(value: number) { }
}

// INTERFACE
@anno
interface TestInterface {
    // INTERFACE_PROPERTY
    @anno
    interfaceProperty: number;

    // INTERFACE_METHOD
    @anno
    interfaceMethod(): void;

    // INTERFACE_GETTER
    @anno
    get interfaceGetter(): number;

    // INTERFACE_SETTER
    @anno
    set interfaceSetter(value: number);
}

// STRUCT
@anno
struct TestStruct { }

// ============================================================================
// Other Targets
// ============================================================================

// LAMBDA
@anno
(a: number): number => a + 1;

// TYPE (generic type parameter and type annotation)
class GenericClass<@anno T > {
    genericField: @anno T;
}

// PARAMETER (function parameter and lambda parameter)
function genericFunction<T>(@anno param: T): void { }

let lambdaParameter = (@anno a: number, @anno b: number): number => a + b;
/* @@? 25:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on NAMESPACE. It is only allowed on: INTERFACE_METHOD. */
/* @@? 27:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on VARIABLE. It is only allowed on: INTERFACE_METHOD. */
/* @@? 32:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on VARIABLE. It is only allowed on: INTERFACE_METHOD. */
/* @@? 36:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on TYPE_ALIAS. It is only allowed on: INTERFACE_METHOD. */
/* @@? 40:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on ENUMERATION. It is only allowed on: INTERFACE_METHOD. */
/* @@? 47:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on FUNCTION. It is only allowed on: INTERFACE_METHOD. */
/* @@? 55:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on FUNCTION_WITH_RECEIVER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 59:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS. It is only allowed on: INTERFACE_METHOD. */
/* @@? 62:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS_FIELD. It is only allowed on: INTERFACE_METHOD. */
/* @@? 66:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS_METHOD. It is only allowed on: INTERFACE_METHOD. */
/* @@? 70:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS_GETTER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 76:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS_SETTER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 81:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE. It is only allowed on: INTERFACE_METHOD. */
/* @@? 84:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_METHOD. */
/* @@? 92:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_GETTER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 96:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_SETTER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 101:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS. It is only allowed on: INTERFACE_METHOD. */
/* @@? 102:1 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */
/* @@? 109:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on LAMBDA. It is only allowed on: INTERFACE_METHOD. */
/* @@? 113:21 Error Semantic error ESE495032: Annotation 'anno' cannot be used on TYPE. It is only allowed on: INTERFACE_METHOD. */
/* @@? 114:20 Error Semantic error ESE495032: Annotation 'anno' cannot be used on TYPE. It is only allowed on: INTERFACE_METHOD. */
/* @@? 118:30 Error Semantic error ESE495032: Annotation 'anno' cannot be used on PARAMETER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 120:25 Error Semantic error ESE495032: Annotation 'anno' cannot be used on PARAMETER. It is only allowed on: INTERFACE_METHOD. */
/* @@? 120:42 Error Semantic error ESE495032: Annotation 'anno' cannot be used on PARAMETER. It is only allowed on: INTERFACE_METHOD. */
