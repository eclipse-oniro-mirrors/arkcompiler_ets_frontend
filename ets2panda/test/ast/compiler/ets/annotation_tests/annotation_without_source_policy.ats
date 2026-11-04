/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

@interface MyAnno{} 

@MyAnno()
type Str = string

@MyAnno()
let a = 1

function foo(){
    @MyAnno()
    const a = 1
}

namespace NS {
    @MyAnno()
    let a = 1
}

@MyAnno()
()=>{}

let b:@MyAnno() string

@Retention("BYTECODE")
@interface Anno{}

@Anno()
()=>{}

function foo(@Anno() a:number){} // ok

/* @@? 18:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 21:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 25:6 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 30:6 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 34:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 37:8 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */
/* @@? 42:2 Error Semantic error ESE0005: Annotations without 'SOURCE' cannot be used on lambda expressions, local declarations, or types. */