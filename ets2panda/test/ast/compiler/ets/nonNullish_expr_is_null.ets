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
class arkts_p {
    field: int;

    static run(out: Console): int {
        let x: arkts_p | null = null;
        let a:int = 0;
        let res: int;
        try {
            res = (x!.field += ++a);
        } catch (e) {
            if (e instanceof NullPointerError) {
                if (a === 0) {
                    return 0;
                }
                return 1;
            }
            return 2;
        }
        return 3;
    }
}

function test() {
    const result = arkts_p.run(console);
    arktest.assertEQ(result,0);
}

test()

/* @@? 23:19 Warning Warning: Bad operand type, the operand of the non-nullish expression is 'null' or 'undefined'. */
/* @@? 23:20 Warning Warning: Bad operand type, the operand of the non-nullish expression is 'null' or 'undefined'. */