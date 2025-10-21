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

@interface AnnoSrc.ClassAuthor {
    mutilArray:number[][] = [
        [...(()=>{})]
    ]
}

@ClassAuthor
function main() {}

/* @@? 16:19 Error SyntaxError: Expected '{', got '.'. */
/* @@? 16:32 Error SyntaxError: Missing type annotation for property 'ClassAuthor'. */
/* @@? 16:32 Error SyntaxError: Identifier expected, got '{'. */
/* @@? 17:29 Error SyntaxError: Invalid value for annotation field, expected a constant literal. */
/* @@? 22:2 Error TypeError: Cannot find type 'ClassAuthor'. */
