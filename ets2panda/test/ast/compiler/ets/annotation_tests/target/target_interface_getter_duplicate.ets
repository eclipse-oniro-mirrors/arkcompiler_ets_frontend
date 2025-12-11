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

@Target({targets: [AnnotationTargets.VARIABLE, AnnotationTargets.INTERFACE_GETTER, AnnotationTargets.INTERFACE_SETTER, AnnotationTargets.INTERFACE_GETTER]})
@Retention("SOURCE")
@interface anno{ }

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

@anno
let topLevelVar: number = 1;
/* @@? 16:120 Error Semantic error ESE263981: Annotation target 'INTERFACE_GETTER' is duplicated. */
/* @@? 21:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE. It is only allowed on: VARIABLE, INTERFACE_GETTER, INTERFACE_SETTER. */
/* @@? 24:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_PROPERTY. It is only allowed on: VARIABLE, INTERFACE_GETTER, INTERFACE_SETTER. */
/* @@? 28:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_METHOD. It is only allowed on: VARIABLE, INTERFACE_GETTER, INTERFACE_SETTER. */
