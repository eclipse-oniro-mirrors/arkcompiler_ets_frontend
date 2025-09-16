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

class C {
    public foo(fn: () => void): void {
         let bar: () => void = (): void => { fn()  }
         for (let j = 0; j < 1; ++j) {}
    }
}

function foo(arr: Object[]): void {
	const copy: () => void = (): void => {
		let c = arr.length
	}
	for (let s = 1; s < 10; s++) {}
}

function foo2(arr: Object[]): void {
	const copy: () => void = (): void => {
		let c = arr.length
	}
	let s: int
	for (let s = 1; s < 10; s++) {}
}


function bar(): void {
	let arr: short[] = [1,2,3,4]
    let fn: (v: short) => void = (v: short): void => {
        arr[0] = (v + 1).toShort();
    }
    for (let arr = 0; arr < 4; ++arr) { }
    arr = [3];
}



function main() {
    let arr: short[] = [1,2,3,4]
    let fn: (v: short) => void = (v: short): void => {
        arr[0] = (v + 1).toShort();
    }
    for (let i = 0; i < 4; ++i) { }
}
