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

@interface Component {

}

@interface Prop {

}

@interface BuilderParam {

}

@interface State {

}

class A {
    count: number
    text: string
}


class C {
    color(x: string) { }
    width(x: int) { }
    fontSize(x: int) { }
}

function Column(x: () => void): C {
    return new C()
}

function Text(x: string) {
    return new C()
}


function f1(x: A, y: ()=>void): C {
    return new C()
}

@Component
struct First {
    @State
    foo: number = 10
    @State
    message: string = "Hi"
    build() {
        f1({count: this.foo, text: this.message}) {
            Text("Hello")
        }.fontSize(42)
    }
}

@Component
struct Second {
    count: int = 10
    @Prop
    text: string = "Hi"
    @BuilderParam
    aBuilder = ()=>{}
    build() {
        Column() {
            Text(this.text).width(this.count)
            this.aBuilder()
        }.color("#000000")
    }
}



/* @@? 58:1 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */
/* @@? 71:1 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */