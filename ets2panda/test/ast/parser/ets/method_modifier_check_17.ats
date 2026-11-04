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

abstract class A {
	instance_method (v:Int): void {}
	abstract instance_method (): void
}
class Base extends A {
	instance_method () {
	  console.log(`Base: instance_method() ...`)
	  super.instance_method(1)
		/* @@ label */super.instance_method() // Should be CTE as instance_method() is abstract
	}
}

let a: A = new Base
a.instance_method ()

function main() {}


/* @@@ label Error Semantic error ESE0026: Cannot call abstract method! */