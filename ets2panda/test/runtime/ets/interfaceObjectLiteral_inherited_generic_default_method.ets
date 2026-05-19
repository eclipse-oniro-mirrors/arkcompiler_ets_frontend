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

interface I {
    meth<T>(p: T): string
}

interface J extends I {
    meth<T>(p: T) { return "j" }
}

let i: I = {} as J

function main() {
    arktest.assertEQ(i.meth(1), "j")
    arktest.assertEQ(i.meth<string>(""), "j")
}
