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

const enum E {
	A = 10,
	B = 20,
	C = 30
}

const enum E1 {
	A = "A",
	B = "B"
}

@interface Anno {
	a: number
	b = [(10 + 3), E.A]
	c: string
	d: boolean
	e: E[] = [E.A, E.B, E.C]
	f: number[]
	g: number[][][]
	h: E
	i: E[][][]
	j: E1
	k: E1[][][]
}


@Anno({a: E.A + 10, c: "a" + "b", d: (1 === 1), f: [], g: [[[0]]], h: E.A, i: [], j: E1.B, k: []})
class C{
	@Anno({a: 10, b: [1, 2, 3], c: "cde", d: true, f: [1], g:[[[0], [1]]], h: E.A, i: [[[E.A], [E.B]]], j: E1.B, k: [[[E1.A], [E1.B]]]})
	public foo() {}
}


/* @@? 29:4 Error Syntax error ESY0001: Missing type annotation for property 'b'. */
/* @@? 42:11 Error Semantic error ESE0012: Invalid value for annotation field, expected a constant literal. */
/* @@? 44:16 Error Semantic error ESE0042: Invalid annotation field type. Only numeric, boolean, string, enum, or arrays of these types are permitted for annotation fields. */