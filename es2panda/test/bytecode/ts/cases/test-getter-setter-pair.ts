/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Accessors produced by the TypeScript transformer flow through the same
// merge gate as JavaScript ones.

class TsPair {
    private v: number = 1;
    get x(): number { return this.v }
    set x(v: number) { this.v = v }
}

const o = {
    get bar(): number { return 2 },
    set bar(v: number) { }
};
