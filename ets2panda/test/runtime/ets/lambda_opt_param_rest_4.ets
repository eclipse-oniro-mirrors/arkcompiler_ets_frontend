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


function foo(f: (a1: string, ...args: string[]) => string) {
    let a0 =  f("a0")
    arktest.assertEQ(a0,  "a0-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a1 =  f("a0", "a1")
    arktest.assertEQ(a1,  "a0-a1-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a2 =  f("a0", "a1", "a2")
    arktest.assertEQ(a2,  "a0-a1-a2-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a3 =  f("a0", "a1", "a2", "a3")
    arktest.assertEQ(a3,  "a0-a1-a2-a3-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a4 =  f("a0", "a1", "a2", "a3", "a4")
    arktest.assertEQ(a4,  "a0-a1-a2-a3-a4-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a5 =  f("a0", "a1", "a2", "a3", "a4", "a5")
    arktest.assertEQ(a5,  "a0-a1-a2-a3-a4-a5-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a6 =  f("a0", "a1", "a2", "a3", "a4", "a5", "a6")
    arktest.assertEQ(a6,  "a0-a1-a2-a3-a4-a5-a6-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a7 =  f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7")
    arktest.assertEQ(a7,  "a0-a1-a2-a3-a4-a5-a6-a7-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a8 =  f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8")
    arktest.assertEQ(a8,  "a0-a1-a2-a3-a4-a5-a6-a7-a8-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a9 =  f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9")
    arktest.assertEQ(a9,  "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a10 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10")
    arktest.assertEQ(a10, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a11 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11")
    arktest.assertEQ(a11, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-undefined-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a12 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12")
    arktest.assertEQ(a12, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-undefined-undefined-undefined-undefined-undefined-undefined-undefined")
    let a13 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13")
    arktest.assertEQ(a13, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-undefined-undefined-undefined-undefined-undefined-undefined")
    let a14 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14")
    arktest.assertEQ(a14, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-undefined-undefined-undefined-undefined-undefined")
    let a15 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15")
    arktest.assertEQ(a15, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-a15-undefined-undefined-undefined-undefined")
    let a16 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15", "a16")
    arktest.assertEQ(a16, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-a15-a16-undefined-undefined-undefined")
    let a17 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15", "a16", "a17")
    arktest.assertEQ(a17, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-a15-a16-a17-undefined-undefined")
    let a18 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15", "a16", "a17", "a18")
    arktest.assertEQ(a18, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-a15-a16-a17-a18-undefined")
    let a19 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15", "a16", "a17", "a18", "a19")
    arktest.assertEQ(a19, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-a15-a16-a17-a18-a19")
    let a20 = f("a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15", "a16", "a17", "a18", "a19", "a20")
    arktest.assertEQ(a20, "a0-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10-a11-a12-a13-a14-a15-a16-a17-a18-a19")
}

function main() {
    let f1 = ((a0: string,
                a1?: string,
                a2?: string,
                a3?: string,
                a4?: string,
                a5?: string,
                a6?: string,
                a7?: string,
                a8?: string,
                a9?: string,
                a10?: string,
                a11?: string,
                a12?: string,
                a13?: string,
                a14?: string,
                a15?: string,
                a16?: string,
                a17?: string,
                a18?: string,
                ...a19: Array<string>): string => {
            let res = a0 + "-" +
                a1 + "-" +
                a2 + "-" +
                a3 + "-" +
                a4 + "-" +
                a5 + "-" +
                a6 + "-" +
                a7 + "-" +
                a8 + "-" +
                a9 + "-" +
                a10 + "-" +
                a11 + "-" +
                a12 + "-" +
                a13 + "-" +
                a14 + "-" +
                a15 + "-" +
                a16 + "-" +
                a17 + "-" +
                a18;
            if(a19.length > 0){
                res += "-" + a19[0];
            } else {
                res += "-undefined"
            }
            return res;
    }) as Function
    foo(f1 as (a1: string, ...args: string[]) => string)
}
