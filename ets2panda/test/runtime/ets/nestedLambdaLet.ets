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

function force(thunk: () => void) {
    thunk();
}

function bool2Levels(): void {
    force(() => {
        let a0 = true;
        force(() => {
            let a1 = a0;
            arktest.assertTrue(a0 && a1)
            a0 = false;
            arktest.assertTrue(!a0 && a1)
            a0 = true;
            a1 = false;
            arktest.assertTrue(a0 && !a1)
        });
    });
}

function bool4Levels(): void {
    force(() => {
        let a0 = true;
        force(() => {
            let a1 = a0;
            force(() => {
                let a2 = a1;
                force(() => {
                    let a3 = a2;
                    arktest.assertTrue(a0 && a1 && a2 && a3)
                    a0 = false;
                    arktest.assertTrue(!a0 && a1 && a2 && a3)
                    a3 = false;
                    arktest.assertTrue(!a0 && a1 && a2 && !a3)
                });
            });
        });
    });
}

function bool4LevelsBi(): void {
    force(() => {
        let a0 = true;
        force(() => {
            let a1 = a0;
            force(() => {
                let a2 = a1;
                force(() => {
                    let a3 = a2;
                    arktest.assertTrue(a0 && a1 && a2 && a3)
                    a0 = false;
                    arktest.assertTrue(!a0 && a1 && a2 && a3)
                    a3 = a0;
                    arktest.assertTrue(!a0 && a1 && a2 && !a3)
                });
                force(() => {
                    a2 = a0;
                    arktest.assertTrue(!a0 && a1 && !a2)
                });
            });
            force(() => {
                arktest.assertTrue(!a0 && a1)
            });
        });
        force(() => {
            arktest.assertTrue(!a0)
        });
    });
}

function number4Levels(): void {
    force(() => {
        let a0 = 1;
        force(() => {
            let a1 = a0++;
            force(() => {
                let a2 = a1++;
                force(() => {
                    let a3 = a2++;
                    a3++;
                    arktest.assertEQ(8, a0 + a1 + a2 + a3)
                });
            });
        });
    });
}

function arrays(): void {
    force(() => {
        let a0 : FixedArray<double> = [1, 0, 0, 0];
        force(() => {
            let a1 = a0;
            a1[1]=a0[0];
            force(() => {
                let a2 = a1;
                a2[2] = a1[1];
                force(() => {
                    let a3 = a2;
                    force(() => {
                        a3[3] = a2[2];
                        arktest.assertEQ(a0[0] + a0[1] + a0[2] + a0[3], 4)
                        a0 = new double[5];
                        arktest.assertEQ(a0.length, 5)
                        arktest.assertEQ(a0[0] + a0[1] + a0[2] + a0[3] + a0[4], 0)
                        arktest.assertEQ(a1[0] + a1[1] + a1[2] + a1[3], 4)
                        arktest.assertTrue(a1 == a2 && a2 == a3)
                    })
                });
            });
        });
    });
}

function main():void {
    bool2Levels();
    bool4Levels();
    bool4LevelsBi();
    number4Levels();
    arrays();
}
