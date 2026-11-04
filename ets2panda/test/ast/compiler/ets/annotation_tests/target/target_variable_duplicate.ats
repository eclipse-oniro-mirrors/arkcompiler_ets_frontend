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

@Target({targets: [AnnotationTargets.VARIABLE, AnnotationTargets.INTERFACE, AnnotationTargets.INTERFACE, AnnotationTargets.VARIABLE, AnnotationTargets.TYPE_ALIAS]})
@Retention("SOURCE")
@interface anno{ }

@anno
interface TestInterface {
    @anno
    interfaceProperty: number;
}

// TYPE_ALIAS
@anno
type TypeAlias = number;

@anno
let topLevelVar: number = 1;
/* @@? 16:77 Error Semantic error ESE263981: Annotation target 'INTERFACE' is duplicated. */
/* @@? 16:106 Error Semantic error ESE263981: Annotation target 'VARIABLE' is duplicated. */
/* @@? 22:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on INTERFACE_PROPERTY. It is only allowed on: VARIABLE, INTERFACE, TYPE_ALIAS. */
