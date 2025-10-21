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

declare namespace MySpace {
    export @interface ClassAuthor {
        AuthorName: string = "aaa"
    }
}


@interface MySpace.ClassAuthor {
    AuthorName: number = 1
}

/* @@? 16:19 Error TypeError: Variable 'MySpace' has already been declared. */
/* @@? 16:19 Error TypeError: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 23:19 Error SyntaxError: Expected '{', got '.'. */
/* @@? 23:32 Error SyntaxError: Missing type annotation for property 'ClassAuthor'. */
/* @@? 23:32 Error SyntaxError: Identifier expected, got '{'. */