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

type TA = [int, number, byte];

function sum1(...numbers: [int, number, byte]): number {
   let res: number = 0;
   for (let x of numbers) {
        if (x instanceof number) {
            res += x
        } else if(x instanceof int || x instanceof byte) {
            res += x.toDouble();
        }
   }
   return res;
}

function sum2(...numbers: TA): number {
   let res: number = 0;
   for (let x of numbers) {
        if (x instanceof number) {
            res += x
        } else if(x instanceof int || x instanceof byte) {
            res += x.toDouble();
        }
   }
   return res;
}

function sum3<T extends [int, number, byte]>(...numbers: T): number {
   let res: number = 0;
   for (let x of numbers) {
        if (x instanceof number) {
            res += x
        } else if(x instanceof int || x instanceof byte) {
            res += x.toDouble();
        }
   }
   return res;
}

function sum4<T extends TA>(...numbers: T): number {
   let res: number = 0;
   for (let x of numbers) {
        if (x instanceof number) {
            res += x
        } else if(x instanceof int || x instanceof byte) {
            res += x.toDouble();
        }
  }
   return res;
}

function main() {
    let arr1: [int, number, byte] = [1, 2, 3];
    let arr2: TA = [1, 2, 3];

    let result1 = sum1(...arr2);
    let result2 = sum2(...arr1);
    let result3 = sum3<TA>(...arr1);
    let result4 = sum4<[int, number, byte]>(...arr2);

    arktest.assertEQ(6, result1);
    arktest.assertEQ(result1, result2);
    arktest.assertEQ(result2, result3);
    arktest.assertEQ(result3, result4);
}
