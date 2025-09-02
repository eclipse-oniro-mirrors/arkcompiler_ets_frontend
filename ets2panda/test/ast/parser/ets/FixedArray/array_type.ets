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

class array_type  {
    private /* @@ label */a : FixedArray<int> ;
    protected /* @@ label1 */f : FixedArray<FixedArray<FixedArray<float>>> ;
    public  foo(s : FixedArray<FixedArray<String>>): void {
    }
    toCharArray/* @@ label2 */(): FixedArray<char> {
    }
    private toStringArray/* @@ label3 */(): FixedArray<String> {
    }
}

/* @@@ label Error TypeError: Property 'a' might not have been initialized.  */
/* @@@ label1 Error TypeError: Property 'f' might not have been initialized.  */
/* @@@ label2 Error TypeError: Function with a non void return type must return a value.  */
/* @@@ label3 Error TypeError: Function with a non void return type must return a value.  */
