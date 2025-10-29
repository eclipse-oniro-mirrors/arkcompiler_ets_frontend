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
 
class Base {
    constructor(...args) {

    }
}

class Derived extends Base{
	constructor(){
		super(3);
	}
}

class Base2 {
    my_func(...args) {

    }
}

class Derived2 extends Base2{
	constructor(){
		my_func(3);
	}
}


/* @@? 17:17 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 17:24 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 29:13 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 29:20 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 36:3 Error Semantic error ESE0145: Property 'my_func' must be accessed through 'this' */