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

/* @@? 17:17 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 17:19 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 17:19 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 17:19 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 18:22 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 18:22 Error Semantic error ESE0144: Type name 'string' used in the wrong context */
/* @@? 21:5 Error Syntax error ESY0224: Identifier expected, got 'function'. */
/* @@? 22:16 Error Semantic error ESE0062: need to specify target type for class composite */
/* @@? 34:1 Error Syntax error ESY0228: Unexpected token, expected '}'. */
