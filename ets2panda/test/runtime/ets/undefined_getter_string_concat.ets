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

class Tmp {
    und_: undefined = undefined;

    get und(): undefined {
        return this.und_;
    }

    concatWithSuffix(suffix: string): string {
        return this.und + suffix;
    }

    concatWithPrefix(prefix: string): string {
        return prefix + this.und;
    }
}

function main(): void {
    let t = new Tmp();

    arktest.assertEQ(t.concatWithSuffix("xx"), "undefinedxx");
    arktest.assertEQ(t.concatWithPrefix("xx"), "xxundefined");
    arktest.assertEQ(t.und + "_suffix", "undefined_suffix");
    arktest.assertEQ("prefix_" + t.und, "prefix_undefined");
}
