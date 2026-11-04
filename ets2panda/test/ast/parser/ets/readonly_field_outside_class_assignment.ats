/*
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

class A {
    readonly a: int = 0;
    static readonly b: int = 1;
}

function mutateParam(obj: A) {
    /* @@ label1 */obj.a = 1;
}

function mutateAlias() {
    let obj = new A();
    let alias = obj;
    /* @@ label2 */alias.a = 2;
}

function mutateClosure(obj: A) {
    let writer = (): void => {
        /* @@ label3 */obj.a = 3;
    };

    writer();
}

function mutateStatic() {
    /* @@ label4 */A.b = 4;
}

function main() {
    let obj = new A();
    mutateParam(obj);
    mutateAlias();
    mutateClosure(obj);
    mutateStatic();
}

/* @@@ label1 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label2 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label3 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label4 Error Semantic error ESE4002: Cannot assign to a readonly field b */
