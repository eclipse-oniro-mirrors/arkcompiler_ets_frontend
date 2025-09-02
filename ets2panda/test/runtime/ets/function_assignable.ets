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

function ccc(): string {
    return "function ccc"
}

let foo: () => string = () => { return "lambda foo" }

interface AAAOp {
    aa?: () => string
}

class A {
    _aa?: () => string
    get aa(): () => string {
        return this._aa!;
    }
    set aa(value: () => string) {
        this._aa = value;
    }
    _init1(it?: AAAOp) {
        this._aa = it?.aa ?? ccc;
    }
    _init2(it?: AAAOp) {
        this.aa = it?.aa ?? ccc;
    }
    _init3() {
        this._aa = foo ?? ccc;
    }
    _init4() {
        this.aa = ccc ?? foo;
    }
    _init5(it?: AAAOp) {
        this._aa = it == undefined ? ccc:it.aa;
    }
    _init6(it?: AAAOp) {
        this._aa = it?.aa || ccc;
    }
}

function main(){
    let a = new A();
    a._init1()
    arktest.assertEQ(a.aa(),"function ccc")
    a._init2()
    arktest.assertEQ(a.aa(),"function ccc")
    a._init3()
    arktest.assertEQ(a.aa(),"lambda foo")
    a._init4()
    arktest.assertEQ(a.aa(),"function ccc")
    a._init5()
    arktest.assertEQ(a.aa(),"function ccc")
    a._init6()
    arktest.assertEQ(a.aa(),"function ccc")
}
