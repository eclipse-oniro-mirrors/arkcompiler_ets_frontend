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
    a1 : string = ""
    a2 : number = 0
}


function main() {
    let r1 = new Array<A>()
    r1.push({a1:"A", a2: 1}, {a1:"B", a2: 2});
    arktest.assertEQ(r1[0].a1, "A")
    arktest.assertEQ(r1[0].a2, 1)
    arktest.assertEQ(r1[1].a1, "B")
    arktest.assertEQ(r1[1].a2, 2)

    let r2 = new Array<Record<string, number>>();
    r2.push({"A": 1}, {"B": 2})
    let v = r2.pop()
    arktest.assertEQ(v!["B"], 2)
    v = r2.pop()
    arktest.assertEQ(v!["A"], 1)

    let r3: [Record<number, string>, Record<string, (p:number) => string>] = [{1:"1", 2:"2", 3:"3"},
        {"A":(p:number):string=>"A" + p, "B":(p:number):string => "b" + p}]
    arktest.assertEQ(r3[0][1], "1")
    arktest.assertEQ(r3[0][2], "2")
    arktest.assertEQ(r3[0][3], "3")
    arktest.assertEQ(r3[1]["A"]!(123), "A123")
    arktest.assertEQ(r3[1]["B"]!(123), "b123")
}