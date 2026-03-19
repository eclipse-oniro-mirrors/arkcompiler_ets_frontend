/**
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

'use static'
export interface I {
	name: string;
}

export class A implements I {
	name: string;
}

export declare class B {
	name: string;
}

export declare class C {
	get name(): string;
}

export declare class D {
	set name(value: string);
}

export declare class E {
	get name(): string;
	set name(value: string);
}

export declare interface F {
	get name(): string;
	set name(value: string);
}

export declare interface G {
	varA: string
	get varB(): string;
	get varC(): number;
	set varC(value: number);
	foo(): void;
	bar: () => void;
}

export declare class H implements G {
	varA: string
	get varB(): string;
	get varC(): number;
	set varC(value: number);
	foo(): void;
	bar: () => void;
}

export declare class J {
	varA: string
	get varB(): string;
	get varC(): number;
	set varC(value: number);
	foo(): void;
	bar: () => void;
}

export declare class K extends J {
	varA: string
	get varB(): string;
	get varC(): number;
	set varC(value: number);
	foo(): void;
	bar: () => void;
}
