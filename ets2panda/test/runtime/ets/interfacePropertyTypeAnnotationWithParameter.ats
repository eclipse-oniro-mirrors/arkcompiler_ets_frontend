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

interface Itf {
    callback1: (temp: string) => string
    callback2: (temp: Array<string>) => Int
    callback3: () => Int
    callback4: ((temp: Array<string>) => Int) | number | string
    callback5: [(temp: number) => void]
    p1: string
    p2: Int
}

class Test implements Itf {
    public callback1: (temp: string) => string = (temp: string): string => {
        return temp;
    }
    public callback2: (temp: Array<string>) => Int = (temp: Array<string>): Int => {
        return 2;
    }
    public callback3: () => Int = (): Int => {
        return 3;
    }
    public callback4: ((temp: Array<string>) => Int) | number | string = 4
    public callback5: [(temp: number) => void] = [(temp: number): void => { arktest.assertEQ(temp, 5); }]
    public p1: string = "p1"
    public p2: Int = 2
}

function main() {
    let a = new Test;
    let b: Array<string> = new Array<string>("hello");

    arktest.assertEQ(a.callback1("hello1"), "hello1");
    arktest.assertEQ(a.callback2(b), 2);
    arktest.assertEQ(a.callback3(), 3);
    arktest.assertEQ(a.callback4, 4);
    a.callback5[0](5);
    arktest.assertEQ(a.p1, "p1");
    arktest.assertEQ(a.p2, 2);
}
