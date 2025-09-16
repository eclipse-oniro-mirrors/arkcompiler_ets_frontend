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
@interface MyAnno {
    testProperty1: string
}

function foo(MyAnno({testProperty1: ""}) x: int) {
}

function foo(MyAnno({testProperty1: ""}) x: int, MyAnno({testProperty1: ""}) y: string) {
}

function foo(MyAnno({testProperty1: ""}) x: int, MyAnno({testProperty1: ""}) y: string) {
}

/* @@? 19:10 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 19:20 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 19:20 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 19:20 Error TypeError: need to specify target type for class composite */
/* @@? 19:42 Error SyntaxError: Unexpected token 'x'. */
/* @@? 19:45 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 19:45 Error SyntaxError: Unexpected token 'int'. */
/* @@? 19:48 Error SyntaxError: Unexpected token ')'. */
/* @@? 19:50 Error SyntaxError: Unexpected token '{'. */
/* @@? 22:1 Error TypeError: Function foo is already declared. */
/* @@? 22:20 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 22:20 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 22:20 Error TypeError: need to specify target type for class composite */
/* @@? 22:42 Error SyntaxError: Unexpected token 'x'. */
/* @@? 22:45 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 22:45 Error SyntaxError: Unexpected token 'int'. */
/* @@? 22:48 Error SyntaxError: Unexpected token ','. */
/* @@? 22:50 Error SyntaxError: Unexpected token 'MyAnno'. */
/* @@? 22:50 Error TypeError: This expression is not callable. */
/* @@? 22:50 Error TypeError: Annotation missing '@' symbol before annotation name. */
/* @@? 22:78 Error SyntaxError: Unexpected token 'y'. */
/* @@? 22:81 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 22:81 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 22:87 Error SyntaxError: Unexpected token ')'. */
/* @@? 22:89 Error SyntaxError: Unexpected token '{'. */
/* @@? 25:1 Error TypeError: Function foo is already declared. */
/* @@? 25:20 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 25:20 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 25:20 Error TypeError: need to specify target type for class composite */
/* @@? 25:42 Error SyntaxError: Unexpected token 'x'. */
/* @@? 25:45 Error SyntaxError: Unexpected token 'int'. */
/* @@? 25:45 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 25:48 Error SyntaxError: Unexpected token ','. */
/* @@? 25:50 Error SyntaxError: Unexpected token 'MyAnno'. */
/* @@? 25:50 Error TypeError: Annotation missing '@' symbol before annotation name. */
/* @@? 25:50 Error TypeError: This expression is not callable. */
/* @@? 25:78 Error SyntaxError: Unexpected token 'y'. */
/* @@? 25:81 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 25:81 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 25:87 Error SyntaxError: Unexpected token ')'. */
/* @@? 25:89 Error SyntaxError: Unexpected token '{'. */