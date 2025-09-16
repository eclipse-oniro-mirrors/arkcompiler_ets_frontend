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

function force(thunk: ()=>void) {
    thunk();
}

function bool2Levels(): void {
    force(() => {
        const a0 = true;
        force(() => {
            const a1 = a0;
            arktest.assertTrue(a0 && a1)
        });
    });
}

function bool4Levels(): void {
    force(() => {
        const a0 = true;
        force(() => {
            const a1 = a0;
            force(()=>{
                const a2 = a1;
                force(()=>{
                    const a3 = a2;
                    arktest.assertTrue(a0 && a1 && a2 && a3)
                });
            });
        });
    });
}

function number4Levels(): void {
    force(() => {
        const a0 = 1;
        force(() => {
            const a1 = a0;
            force(()=>{
                const a2 = a1;
                force(()=>{
                    const a3 = a2;
                    arktest.assertEQ(4, a0 + a1 + a2 + a3)
                });
            });
        });
    });
}

function array4Levels(): void {
    force(() => {
        const a0 = [1,0,0,0];
        force(() => {
            const a1 = a0;
            a1[1]=a0[0];
            force(()=>{
                const a2 = a1;
                a2[2]=a1[1];
                force(()=>{
                    const a3 = a2;
                    a3[3]=a2[2];
                    arktest.assertEQ(a0[0]+a0[1]+a0[2]+a0[3], 4)
                });
            });
        });
    });
}

function arrayProp4Levels(): void {
    force(() => {
        const a0 = [1,0,0,0];
        force(() => {
            const a1 = a0;
            a1[1]=a0[0];
            force(()=>{
                const a2 = a1;
                a2[2]=a1[1];
                force(()=>{
                    const a3 = a2;
                    a3[3]=a2[2];
                    arktest.assertEQ(a0.length, 4)
                    arktest.assertEQ(a0[0]+a0[1]+a0[2]+a0[3], 4)
                });
            });
        });
    });
}

function paramCapture(): void {
    const v0 = 1
    const f0 = (p0:number)=>{
        const a0 = p0;
        const f1 = (p1:number)=>{
            const a1 = a0;
            const a2 = p1;
            const a3 = v0;
            arktest.assertEQ(a0 + a1 + a2 + a3, 4)
        }
        f1(p0);
    }
    f0(v0);
}


function main():void {
    bool2Levels();
    bool4Levels();
    number4Levels();
    array4Levels();
    paramCapture();
}
