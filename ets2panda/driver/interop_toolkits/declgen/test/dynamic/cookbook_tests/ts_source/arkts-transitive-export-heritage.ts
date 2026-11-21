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

// 3a: exported class extends a local (non-exported) class
class Base {
    field: string = '';
}
// 3b: exported class implements a local (non-exported) interface
interface IFace {
    method(): void;
}
// 3c: chain — Mid extends Base, Derived extends Mid (fixpoint must reach Base)
class Mid extends Base {
    mid: string = '';
}
export class Derived extends Mid implements IFace {
    method(): void {}
}
