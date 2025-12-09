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

interface II { name() : string }

class A {
    protected name(): string {
        return "I am A"
    }
}

class B {
    final name(): string {
        return "I am B"
    }
}

class C {
    private name(): string {
        return "I am C"
    }
}

class D {
    static name(): string {
        return "I am C"
    }
}

function f1(x: II | A) {
    x.name();
}

function f2(x: A | B) {
    return x.name();
}

function f3(x: B | D) {
    x.name();
}

/* @@? 43:5 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@? 43:5 Error Semantic error ESE0127: No matching call signature for name() */
/* @@? 47:12 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@? 47:12 Error Semantic error ESE0127: No matching call signature for name() */
/* @@? 51:5 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@? 51:5 Error Semantic error ESE0127: No matching call signature for name() */
/* @@? 51:7 Error Semantic error ESE0207: 'name' is a static property of 'D' */
