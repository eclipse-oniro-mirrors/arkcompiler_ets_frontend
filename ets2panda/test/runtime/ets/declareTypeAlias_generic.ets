/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

// Union
export type type_union<T> = T | boolean;


// Array
export type type_array_1<T> = Array<T>;
type type_array_2<T> = T[];


// Array + Union
export type type_union_array_1<T, U> = Array<T> | Array<U>
type type_union_array_2<T, U> = Array<T | U>


// Array + Type Alias
export type type_array_alias_1<T> = Array<type_union<T>>
type type_array_alias_2<T> = Array<type_union<T>> | type_union<T>
type type_array_alias_3<T> = Array<type_array_1<T>> | type_array_2<T>


// Tuple
type type_tuple_1<T, U, R> = [T, U, R]
type type_tuple_2<T, U, R> = [T, Array<U>, R]
type type_tuple_3<T, U, R, P> = [T, [U, R], P]
type type_tuple_4<T, U, R> = [T, U[], R]

// Tuple + Union
type type_tuple_union_1<T> = [string, T | null, number]
type type_tuple_union_2<T> = [string, Array<T> | boolean, number]
type type_tuple_union_3<T> = [string, T, number] | Array<T> | [string, T, number]
type type_tuple_union_4<T> = [string, T, number] | (T | undefined)[]

// Tuple + Type Alias
type type_tuple_alias_1<T, U> = [T, U | null, type_union_array_1<T, U>]
type type_tuple_alias_2 = [string, Array<boolean> | boolean, type_union_array_1<string, boolean>]
type type_tuple_alias_3<T, U, R> = [string, U, R] | Array<boolean> | [string, boolean, number] | type_tuple_union_4<T>
type type_tuple_alias_4 = [string, boolean, number] | (boolean | undefined)[] | type_tuple_union_3<string>


// Utility
interface Issue<T> {
	title : T
	description : T
}

interface Issue2<T> {
	title?: T
	description?: string
}
type type_utility_1 = Required<Issue2<string>>;
type type_utility_2<T> = Readonly<Issue<T>>;
type type_utility_3<T> = Record<number, T>;


function main() {
	let type_union_var : type_union<string> = "name"
	let type_array_var : type_array_1<boolean> = new Array<boolean>()
	let type_union_array_var : type_union_array_1<string, string> = new Array<string>()
	let type_array_alias_var : type_array_alias_1<char> = new Array<char | boolean>()
	let type_tuple_var : type_tuple_1<string, boolean, number> = ["name", true, 2]
	let type_tuple_union_var : type_tuple_union_2<undefined> = ["name", true, 2]
	let type_tuple_alias__var : type_tuple_alias_1<string, string> = ["name", null, new Array<string>]
	let type_utility_1_var : type_utility_1 = {title: "type alias", description: "test for type alias"}
	let type_utility_2_var : type_utility_2<string> = {title: "type alias", description: "test for type alias"}
	let type_utility_3_var : type_utility_3<Object> = {
		1 : new Object()
	}
}

