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

// ============================================================================
// Top-Level Declarations
// ============================================================================

// NAMESPACE
@Target([AnnotationTargets.CLASS])
namespace TestNamespace {
    @Target([AnnotationTargets.CLASS])
    let namespaceVar: number = 1;
}

// VARIABLE
@Target([AnnotationTargets.CLASS])
let topLevelVar: number = 1;

// TYPE_ALIAS
@Target([AnnotationTargets.CLASS])
type TypeAlias = number;

// ENUMERATION
@Target([AnnotationTargets.CLASS])
enum TestEnum {
    A = 1,
    B = 2
}

// FUNCTION
@Target([AnnotationTargets.CLASS])
function testFunction(): void { }

// FUNCTION_WITH_RECEIVER (extension function)
class ReceiverClass {
    value: number = 1;
}

@Target([AnnotationTargets.CLASS])
function testExtension(this: ReceiverClass): void { }

// CLASS
@Target([AnnotationTargets.CLASS])
class TestClass {
    // CLASS_FIELD
    @Target([AnnotationTargets.CLASS])
    classField: number = 1;

    // CLASS_METHOD
    @Target([AnnotationTargets.CLASS])
    classMethod(): void { }

    // CLASS_GETTER
    @Target([AnnotationTargets.CLASS])
    get classGetter(): number {
        return 1;
    }

    // CLASS_SETTER
    @Target([AnnotationTargets.CLASS])
    set classSetter(value: number) { }
}

// INTERFACE
@Target([AnnotationTargets.CLASS])
interface TestInterface {
    // INTERFACE_PROPERTY
    @Target([AnnotationTargets.CLASS])
    interfaceProperty: number;

    // INTERFACE_METHOD
    @Target([AnnotationTargets.CLASS])
    interfaceMethod(): void;

    // INTERFACE_GETTER
    @Target([AnnotationTargets.CLASS])
    get interfaceGetter(): number;

    // INTERFACE_SETTER
    @Target([AnnotationTargets.CLASS])
    set interfaceSetter(value: number);
}

// STRUCT
@Target([AnnotationTargets.CLASS])
struct TestStruct { }

// ============================================================================
// Other Targets
// ============================================================================

// LAMBDA
@Target([AnnotationTargets.CLASS])
(a: number): number => a + 1;

// TYPE (generic type parameter and type annotation)
class GenericClass<@Target([AnnotationTargets.CLASS]) T > {
    genericField: @Target([AnnotationTargets.CLASS]) T;
}

// PARAMETER (function parameter and lambda parameter)
function genericFunction<T>(@Target([AnnotationTargets.CLASS]) param: T): void { }

let lambdaParameter = (@Target([AnnotationTargets.CLASS]) a: number, @Target([AnnotationTargets.CLASS]) b: number): number => a + b;
/* @@? 21:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 23:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 23:6 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 28:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 28:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 32:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 32:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 36:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 43:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 51:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 55:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 58:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 62:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 66:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 72:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 77:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 80:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 84:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 88:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 92:6 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 97:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 98:1 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */
/* @@? 105:2 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 105:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 109:21 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 109:21 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 110:20 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 110:20 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 114:30 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 116:25 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
/* @@? 116:71 Error Semantic error ESE1122645: Annotation '@Target' can only be applied to annotation declarations. */
