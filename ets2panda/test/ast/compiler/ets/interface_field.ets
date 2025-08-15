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

interface Todo {
    teTodo(todo1, {
        description: string;
    }

    function updateTodo(todo:Todo, fieldToUpdate: Partial<Todo>) {
        return {...todo, ...fieldToUpdate };
    }

/* @@? 17:17 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 17:19 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 17:19 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 17:19 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 18:22 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 18:22 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 21:14 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 21:14 Error SyntaxError: Identifier expected, got 'identification literal'. */
/* @@? 21:25 Error TypeError: Cannot find type 'todo'. */
/* @@? 21:29 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 21:29 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 21:34 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 21:36 Error TypeError: Cannot find type 'fieldToUpdate'. */
/* @@? 21:49 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 21:49 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 21:64 Error SyntaxError: Unexpected token, expected '('. */
/* @@? 21:66 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 21:66 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 21:66 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 22:16 Error TypeError: need to specify target type for class composite */
/* @@? 46:1 Error SyntaxError: Unexpected token, expected '}'. */
