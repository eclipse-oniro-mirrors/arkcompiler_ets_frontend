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

@interface MyValue {[1,2,3]}

class A{
    @MyValue([1,2,3])
    foo(){}
}

/* @@? 16:22 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 16:22 Error SyntaxError: Number, string or computed value property name '1' is not allowed, use classes to access data by property names that are identifiers */
/* @@? 16:22 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 16:22 Error SyntaxError: Number, string or computed value property name '1' is not allowed, use classes to access data by property names that are identifiers */
/* @@? 16:22 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 16:23 Error SyntaxError: Identifier expected, got ','. */
/* @@? 16:24 Error SyntaxError: Number, string or computed value property name '2' is not allowed, use classes to access data by property names that are identifiers */
/* @@? 16:24 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 16:24 Error SyntaxError: Number, string or computed value property name '2' is not allowed, use classes to access data by property names that are identifiers */
/* @@? 16:24 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 16:25 Error SyntaxError: Identifier expected, got ','. */
/* @@? 16:26 Error SyntaxError: Number, string or computed value property name '3' is not allowed, use classes to access data by property names that are identifiers */
/* @@? 16:26 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 16:26 Error SyntaxError: Number, string or computed value property name '3' is not allowed, use classes to access data by property names that are identifiers */
/* @@? 16:26 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 16:27 Error SyntaxError: Identifier expected, got ']'. */
/* @@? 16:28 Error SyntaxError: Identifier expected, got '}'. */
/* @@? 19:6 Error TypeError: Annotation 'MyValue' requires multiple fields to be specified. */