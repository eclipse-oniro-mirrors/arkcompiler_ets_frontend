/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

export class AccessFannkuch {
    public n: int;
    constructor() {
        this.n = 8;
    }

    static expected: int = 22;

    public fannkuch(n: int): int {
        // Not parsed new int[n]
        let perm: int[] = new int[this.n];
        // Not parsed new int[n]
        let perm1: int[] = new int[this.n];
        // Not parsed new int[n]
        let count: int[] = new int[this.n];
        // Not parsed new int[n]
        let maxPerm: int[] = new int[this.n];
        let maxFlipsCount: int = 0;
        let m: int = this.n - 1;

        for (let i: int = 0; i < this.n; i++) perm1[i] = i;
        let r: int = this.n;

        while (true) {
            while (r != 1) {
                count[r - 1] = r;
                r--;
            }
            if (!(perm1[0] == 0 || perm1[m] == m)) {
                for (let i: int = 0; i < this.n; i++) perm[i] = perm1[i];

                let flipsCount: int  = 0;
                let k: int;

                while (!((k = perm[0]) == 0)) {
                let k2: int = (k + 1) >> 1;
                    for (let i = 0; i < k2; i++) {
                        let temp: int = perm[i];
                        perm[i] = perm[k - i];
                        perm[k - i] = temp;
                    }
                    flipsCount++;
                }

                if (flipsCount > maxFlipsCount) {
                    maxFlipsCount = flipsCount;
                    for (let i = 0; i < this.n; i++) maxPerm[i] = perm1[i];
                }
            }

            while (true) {
                if (r == n) return maxFlipsCount;
                let perm0: int = perm1[0];
                let i: int = 0;
                while (i < r) {
                    let j: int = i + 1;
                    perm1[i] = perm1[j];
                    i = j;
                }
                perm1[r] = perm0;

                count[r] = count[r] - 1;
                if (count[r] > 0) break;
                r++;
            }
        }
    }

    public run(): void {
       let ret = this.fannkuch(this.n);
        arktest.assertEQ(ret, AccessFannkuch.expected,  "Invalid result");
    }
}

function main(): void {
    let a = new AccessFannkuch;
    a.run();
}
