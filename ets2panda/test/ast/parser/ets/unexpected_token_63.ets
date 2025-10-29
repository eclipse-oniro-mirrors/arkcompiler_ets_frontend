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

class A {
	func1(list : number, final : number) {
		while(true) {
			if(false) {
				break;
			} else {
				let endSequence = false
				this.func(list, final, endSequence)
			}
		}
	}
}



/* @@? 17:7 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 17:23 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 17:23 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 17:23 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 17:29 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 17:31 Error Syntax error ESY0295: number is a predefined type, cannot be used as an identifier */
/* @@? 17:37 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 17:39 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 18:3 Error Syntax error ESY0227: Unexpected token 'while'. */
/* @@? 18:8 Error Syntax error ESY103145: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 18:9 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 18:9 Error Syntax error ESY0227: Unexpected token 'true'. */
/* @@? 18:9 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 18:9 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 18:13 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 18:15 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 19:4 Error Syntax error ESY0227: Unexpected token 'if'. */
/* @@? 19:6 Error Syntax error ESY103145: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 19:7 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 19:7 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 19:7 Error Syntax error ESY0227: Unexpected token 'false'. */
/* @@? 19:7 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 19:12 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 19:14 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 20:5 Error Syntax error ESY0227: Unexpected token 'break'. */
/* @@? 21:6 Error Syntax error ESY0227: Unexpected token 'else'. */
/* @@? 21:11 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 23:5 Error Semantic error ESE0202: 'this' cannot be referenced from a static context */
/* @@? 23:10 Error Semantic error ESE0087: Property 'func' does not exist on type 'ETSGLOBAL' */
/* @@? 23:21 Error Syntax error ESY0227: Unexpected token 'final'. */
/* @@? 25:3 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 26:2 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 27:1 Error Syntax error ESY0227: Unexpected token '}'. */