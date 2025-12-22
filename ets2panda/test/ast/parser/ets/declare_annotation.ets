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

@interface ClassAuthor {
    au`horName: string = "1"
}

class A{
    @ClassAuthor
    foo1(){}

    @ClassAuthor("11")
    foo2(){}

    @ClassAuthor({authorName: "22"})
    foo3()


/* @@? 17:7 Error Syntax error ESY0001: Missing type annotation for property 'au'. */
/* @@? 17:7 Error Syntax error ESY0224: Identifier expected, got '`'. */
/* @@? 21:6 Error Semantic error ESE0044: The required field 'au' must be specified. Fields without default values cannot be omitted. */
/* @@? 24:6 Error Semantic error ESE0043: Annotation 'ClassAuthor' requires multiple fields to be specified. */
/* @@? 24:18 Error Semantic error ESE0042: Invalid annotation field type. Only numeric, boolean, string, enum, or arrays of these types are permitted for annotation fields. */
/* @@? 27:6 Error Semantic error ESE0044: The required field 'au' must be specified. Fields without default values cannot be omitted. */
/* @@? 27:19 Error Semantic error ESE0045: The parameter 'authorName' does not match any declared property in the annotation 'ClassAuthor'. */
/* @@? 28:9 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 39:79 Error Syntax error ESY0230: Expected '}', got 'end of stream'. */