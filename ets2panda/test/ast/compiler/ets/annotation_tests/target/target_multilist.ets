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

@Target([AnnotationTargets.NAMESPACE, AnnotationTargets.INTERFACE, AnnotationTargets.STRUCT, AnnotationTargets.CLASS,  AnnotationTargets.VARIABLE,
         AnnotationTargets.FUNCTION, AnnotationTargets.PARAMETER, AnnotationTargets.TYPE, AnnotationTargets.TYPE_ALIAS, AnnotationTargets.ENUMERATION,
         AnnotationTargets.FUNCTION_WITH_RECEIVER, AnnotationTargets.CLASS_FIELD, AnnotationTargets.CLASS_METHOD, AnnotationTargets.CLASS_GETTER,
         AnnotationTargets.CLASS_SETTER, AnnotationTargets.INTERFACE_PROPERTY, AnnotationTargets.INTERFACE_METHOD, AnnotationTargets.INTERFACE_GETTER,
         AnnotationTargets.INTERFACE_SETTER, AnnotationTargets.LAMBDA])
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
/* @@? 106:1 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */
