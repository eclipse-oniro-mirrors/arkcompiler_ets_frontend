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

class AA{}
class BB{}

@Retention("SOURCE")
@interface MyTypeAnnotation {}

function main(){
    let x: @MyTypeAnnotation (AA|BB)
}

/* @@? 23:30 Error Syntax error ESY150811: Annotation usage must have parentheses when used in front of a parenthesized type. */
/* @@? 23:30 Error Syntax error ESY0227: Unexpected token '('. */
/* @@? 23:31 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@? 23:34 Error Syntax error ESY0318: Class cannot be used as object. */
