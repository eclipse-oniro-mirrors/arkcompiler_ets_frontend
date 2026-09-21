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

// A pair may merge across differently-named members and fields: none of them
// can change the pair's override order.

class Intervening {
    get a() { return 1 }
    f = 1;
    m() { return "m" }
    set a(v) { }
};
const d = Object.getOwnPropertyDescriptor(Intervening.prototype, "a");
print(typeof d.get, typeof d.set);
print(new Intervening().m(), new Intervening().f);
