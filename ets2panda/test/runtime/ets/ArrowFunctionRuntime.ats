/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
    bar(a0: int): void{
        let func: (a1: int) => void = (a1: int): void => {
            console.print("bar.func");
        }
        this.classf(func);
    }

    functest(func: (a1: int) => void): void {
        func(2);
    }

    sim_print(): void {
        console.print("sim_print");
    }

    classf(locfunc: (a1: int) => void): void {
        this.functest(locfunc);
        this.functest( (a1: int): void => { console.print("lambda_func") } );
        let capture_param: () => void = (): void => {
            locfunc(2);
        }
        let capture_this: () => void = (): void => {
            this.sim_print();
        }
        this.functest((a1: int): void => {
            locfunc(2);
        });
        locfunc(3);
    }
}

function main(): void {
    let testClass: A = new A();
    testClass.bar(2);
}
