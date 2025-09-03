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

function path<T>();
function path(obj: Object, ...keys: (string | number)[]);
function path(obj: Object, ...keys: (string | number)[]) {
}

function f1(thing: string) {
    path(thing, ...['a']);
}

/* @@? 16:10 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 18:1 Error TypeError: Function path with this assembly signature already declared. */
