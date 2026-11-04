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
 
/*
 * Can not moved into ast/compile while #19557 isn't resolved
 */
 
enum Color {
    Red = 0,
    Green = 1,
    Blue = 2
}

enum Shape {
    Circle = "CIRCLE",
    Square = "SQUARE",
    Triangle = "TRIANGLE"
}

class A {
    public sub: Color;
    public shape: Shape;

    constructor(sub: Color, shape: Shape) {
        this.sub = sub;
        this.shape = shape;
    }

    foo() {
        let a = this.sub.valueOf();
        let b = this.shape.valueOf();
    }
}

