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
@interface MyAnno {
    testProperty1: string
    foo(){}
}



/* @@? 17:8 Error Syntax error ESY0025: Annotation can not have method as property. */
/* @@? 17:8 Error Syntax error ESY0001: Missing type annotation for property 'foo'. */
/* @@? 17:8 Error Syntax error ESY0224: Identifier expected, got '('. */
/* @@? 17:9 Error Syntax error ESY0224: Identifier expected, got ')'. */
/* @@? 17:10 Error Syntax error ESY0224: Identifier expected, got '{'. */
/* @@? 17:11 Error Syntax error ESY0224: Identifier expected, got '}'. */
/* @@? 18:1 Error Syntax error ESY0227: Unexpected token '}'. */