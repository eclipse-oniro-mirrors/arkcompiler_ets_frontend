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
/*---
    tags: [compile-only, negative]
---*/

interface Todo {
  title: string;
  teTodo(todo1, {
    descriptio
    description: string;
  }
  tion:"throw out
  function updateTodo(todo:Todo, fieldToUpdate: Partial<Todo>) {
    return {...todo, ...fieldToUpdate };
  }

  const todo1: Todo = {
    title: "original desk",
    de "ription: "clea,
  };

  const todo2 = updateTodo(todo1, {
    description:"throw out trash",
  });
