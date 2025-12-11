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

@Target({targets: [AnnotationTargets.VARIABLE, AnnotationTargets.CLASS_GETTER, AnnotationTargets.CLASS_SETTER, AnnotationTargets.VARIABLE, AnnotationTargets.CLASS_GETTER]})
@Retention("SOURCE")
@interface anno{ }

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

@anno
let topLevelVar: number = 1;
/* @@? 16:112 Error Semantic error ESE263981: Annotation target 'VARIABLE' is duplicated. */
/* @@? 16:140 Error Semantic error ESE263981: Annotation target 'CLASS_GETTER' is duplicated. */
/* @@? 21:2 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS. It is only allowed on: VARIABLE, CLASS_GETTER, CLASS_SETTER. */
/* @@? 24:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS_FIELD. It is only allowed on: VARIABLE, CLASS_GETTER, CLASS_SETTER. */
/* @@? 28:6 Error Semantic error ESE495032: Annotation 'anno' cannot be used on CLASS_METHOD. It is only allowed on: VARIABLE, CLASS_GETTER, CLASS_SETTER. */
