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

let res: string = "";

class TestRecord extends Record<string, float> {
    override get(k: string, def: float): float {
        res += "1";
        return 0;
    }

    override get(k: string): float {
        res += "2"
        return 0;
    }

    override set(k: string, val: float): this {
        res += "3"
        return this;
    }

    override $_get(k: string): float | undefined {
        res += "4"
        return 0;
    }

    override $_set(k: string, v: float): void {
        res += "5"
    }
}

function main() {
    res = ""

    let targets: Record<string, float> = new TestRecord() as Record<string, float>
    targets.get("name", 0)
    targets.get("name", 1.0)
    targets.get("name")
    let name = targets["name"]
    targets["name"] = 1.0

    arktest.assertEQ(res, "11245")
}
