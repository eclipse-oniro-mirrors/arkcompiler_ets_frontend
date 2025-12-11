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

@Target({targets: [AnnotationTargets.NAMESPACE, AnnotationTargets.INTERFACE]})
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

@anno
interface TestInterface {
    @anno
    interfaceProperty: number;
}
/* @@? 27:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on VARIABLE. It is only allowed on: NAMESPACE, INTERFACE. */
/* @@? 33:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_PROPERTY. It is only allowed on: NAMESPACE, INTERFACE. */
