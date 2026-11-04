/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class C<V> {
    constructor(v: V) { this.v = v }
    public v: V
}
function bar<V>(bab: (x: V) => C<V>, x: V): V {
    return bab(x).v
}

function baz<V>(c: () => V): C<V> { return new C < V > (c()); }

function foo<V>(gen: () => V, x: V): V {
    return bar<V>((x) => baz<V>(gen), x)
}
