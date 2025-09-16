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


/* @@? 17:7 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 17:23 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 17:23 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 17:23 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 17:29 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:31 Error SyntaxError: number is a predefined type, cannot be used as an identifier */
/* @@? 17:37 Error SyntaxError: Unexpected token ')'. */
/* @@? 17:39 Error SyntaxError: Unexpected token '{'. */
/* @@? 18:3 Error SyntaxError: Unexpected token 'while'. */
/* @@? 18:8 Error SyntaxError: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 18:9 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 18:9 Error SyntaxError: Unexpected token 'true'. */
/* @@? 18:9 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 18:9 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 18:13 Error SyntaxError: Unexpected token ')'. */
/* @@? 18:15 Error SyntaxError: Unexpected token '{'. */
/* @@? 19:4 Error SyntaxError: Unexpected token 'if'. */
/* @@? 19:6 Error SyntaxError: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 19:7 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 19:7 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 19:7 Error SyntaxError: Unexpected token 'false'. */
/* @@? 19:7 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 19:12 Error SyntaxError: Unexpected token ')'. */
/* @@? 19:14 Error SyntaxError: Unexpected token '{'. */
/* @@? 20:5 Error SyntaxError: Unexpected token 'break'. */
/* @@? 21:6 Error SyntaxError: Unexpected token 'else'. */
/* @@? 21:11 Error SyntaxError: Unexpected token '{'. */
/* @@? 23:5 Error TypeError: 'this' cannot be referenced from a static context */
/* @@? 23:10 Error TypeError: Property 'func' does not exist on type 'ETSGLOBAL' */
/* @@? 23:21 Error SyntaxError: Unexpected token 'final'. */
/* @@? 25:3 Error SyntaxError: Unexpected token '}'. */
/* @@? 26:2 Error SyntaxError: Unexpected token '}'. */
/* @@? 27:1 Error SyntaxError: Unexpected token '}'. */
