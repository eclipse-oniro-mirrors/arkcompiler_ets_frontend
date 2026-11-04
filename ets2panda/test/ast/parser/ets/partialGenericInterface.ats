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

interface inter <T>{
	get value(): Double;
	set value(value: Double);
}

class A <T>{}

class B <T> extends A <T> implements inter <T> {
	value: Double = 1.0;
}

function foo(i: Partial<inter<String>>, a: Partial<A<String>>): void {}


function foa(b: Partial<B<String>>): void {
	foo(b, b);
}
