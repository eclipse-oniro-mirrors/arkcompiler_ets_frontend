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

class A {
    public _num: number = 0;
    get num(): number {
        return this._num;
    }

    set num(n: number) {
        this._num = n;
    }
}

let a: A = /* @@ label1 */{
    _num:0,

    get /* @@ label2 */num(): number {
        return this._num;
    },
    
    set /* @@ label3 */num(n: number) {
        this._num = n;
    }
}


/* @@@ label1 Error Semantic error ESE0064: The object literal properties must be key-value pairs */
/* @@@ label2 Error Syntax error ESY0062: Object pattern can't contain methods. */
/* @@@ label3 Error Syntax error ESY0062: Object pattern can't contain methods. */