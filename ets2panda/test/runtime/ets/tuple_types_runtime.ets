/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

type num_str_str = [number, string, string];
type num_str_str_with_array = [num_str_str, number[]];

class A {
    foo(): int {
        return 1;
    }
}

class B extends A {
    override foo(): int  {
        return 2;
    }
}

class TupleProvider<X> {
    memb: [X, X];
    constructor(value: [X, X]) { this.memb = value;}
    public publish(): [X, X] { return this.memb; }
}


function main(): void {
    const tup_1: [string, number] = ["a", 12];

    arktest.assertEQ(tup_1[0], "a")
    arktest.assertEQ(tup_1[1], 12)

    const tup_2: num_str_str[] = [
        [13, "A", "D"],
        [14, "B", "E"],
        [25, "C", "F"],
    ];

    const tup_3: num_str_str_with_array[] = [
        [tup_2[0], [250]],
        [tup_2[1], [250, 260]],
        [tup_2[0], [300, 300, 300]],
    ];

    const sum = tup_3[0][1][0] + tup_3[1][1][0] + tup_3[1][1][1] + tup_3[2][1][0] + tup_3[2][1][1] + tup_3[2][1][2];
    arktest.assertEQ(sum, 1660);

    tup_3[0][0][0]++;
    arktest.assertEQ(tup_3[0][0][0], 14)
    tup_3[0][0][0] = 4;
    arktest.assertEQ(tup_3[0][0][0], 4)

    let int_update: [int] = [42];
    arktest.assertEQ(int_update[0], 42)
    arktest.assertEQ(int_update[0]++, 42)
    arktest.assertEQ(int_update[0], 43)
    arktest.assertEQ(++int_update[0], 44)
    int_update[0]++;
    arktest.assertEQ(int_update[0], 45)

    let tup_8: [number, string][];
    tup_8 = [[1, "E"], [2, "F"], [3, "G"]];
    arktest.assertTrue(tup_8[0][0] == 1 && tup_8[0][1] == "E")
    arktest.assertTrue(tup_8[1][0] == 2 && tup_8[1][1] == "F")
    arktest.assertTrue(tup_8[2][0] == 3 && tup_8[2][1] == "G")

    let tup_10: [number, int, string, boolean, Object] = [1, 2, "I", false, new Object()];
    let var_float: float = tup_10[1];
    let var_float_2: float = 2.0f;
    arktest.assertEQ(var_float, var_float_2);

    let tup_11: [int, number, string, boolean, Object] = [6, 7, "J", true, 789];
    tup_11[0] += new Short(2 as short);

    arktest.assertEQ(tup_11[0], 8);
    arktest.assertEQ(tup_11[4], (789 as Object));

    let tup_13: [number, number] = [8, 9];
    let tup_14: [number, number] = [10, 11];

    tup_13 = tup_14;
    arktest.assertEQ(tup_13[0], 10)
    arktest.assertEQ(tup_13[1], 11)
    tup_13[0] = 12;
    tup_13[1] = Double.valueOf(13.0);
    arktest.assertEQ(tup_13[0], 12)
    arktest.assertEQ(tup_13[1], 13)

    let a_b_tup: [A, A, B] = [new A(), new B(), new B()];
    arktest.assertEQ(a_b_tup[0].foo(), 1)
    arktest.assertEQ(a_b_tup[1].foo(), 2)
    arktest.assertEQ(a_b_tup[2].foo(), 2)

    let number_tup: [Double, Double] = [42.0, 43.0];
    let tn: TupleProvider<number> = new TupleProvider<number>(number_tup);
    let result: [Number, Number] = tn.publish();
    arktest.assertEQ(result[0], 42.0)
}
