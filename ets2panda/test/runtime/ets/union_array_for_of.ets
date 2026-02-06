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

type UT = C | string | D | Array<int>;

class C implements Iterable<string> {
	data: string[] = ['a', 'b', 'c']
	index = 0
	$_iterator() {
		return new CIt(this);
	}
}

class CIt implements Iterator<string> {
	index = 0
	base: C
	constructor(c: C) {
		this.base = c
	}
	next(): IteratorResult<string> {
		return {
			done: this.index >= this.base.data.length,
			value: this.index >= this.base.data.length ? undefined : this.base.data[this.index++]
		}
	}
}

class D implements Iterable<number> {
	data: number[] = [1, 2, 3]
	index = 0
	$_iterator() {
		return new DIt(this);
	}

}

class DIt implements Iterator<number> {
	index = 0
	base: D
	constructor(d: D) {
		this.base = d
	}
	next(): IteratorResult<number> {
		return {
			done: this.index >= this.base.data.length,
			value: this.index >= this.base.data.length ? undefined : this.base.data[this.index++]
		}
	}
}

function main(): void {
	let arr: UT[] = [new C(), "def", new D(), [4, 5, 6]];
	let res = ''
	for (let y of arr) {
		for (let x of y) {
			res += x;
		}
	}
	arktest.assertEQ(res, 'abcdef123456');
}