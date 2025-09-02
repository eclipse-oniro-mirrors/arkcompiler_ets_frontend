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

<<<<<<<< HEAD:ets2panda/test/ast/compiler/ets/unresolve_class_with_type_infer.ets
function foo<T extends Record<string, A>>(record: Record<string, Partial<T>>){

}

foo({})

/* @@? 16:39 Error TypeError: Cannot find type 'A'. */
========
function main(): void {
    const c = 0;
    /* @@ label */c = c + 1;
}

/* @@@ label Error TypeError: Cannot assign to a constant variable c */
>>>>>>>> origin/OpenHarmony_feature_Release_20250728:ets2panda/test/ast/compiler/ets/assign_const.ets
