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

namespace MySpace {
    // 1. Variable definition and assignment
    export let count = 123;
    export let message = "Initial message";

    // 2. Function definition
    export function resetMessage() {
        message = "Message has been reset";
    }

    // 3. Interface definition
    export interface IShape {
        area(): number;
    }

    // 4. Class definition implementing the interface
    export class Rectangle implements IShape {
        width: number;
        height: number;

        constructor(width: number, height: number) {
            this.width = width;
            this.height = height;
        }

        area(): number {
            return this.width * this.height;
        }
    }

    // 5. Enum definition
    export enum Color {
        RED,
        GREEN,
        BLUE
    }

    // 6. Annotations definition
    export @interface Anno {
        color: Color
    }

    // Reset function to initialize the variables to their default values
    export function resetState() {
        count = 123;
        message = "Initial message";
    }

    // Update values (for testing)
    count = 456;
    resetMessage();
    let shape: IShape = new Rectangle(5, 10);

    @Anno(Color.RED)
    class ColoredRectangle {}

}

// Assertions outside the namespace
MySpace.resetState();  // Reset the state before running arktest.assertions
MySpace.count = 456;
arktest.assertEQ(MySpace.count, 456,  "Count should be updated to 456 outside namespace")

MySpace.resetMessage();
arktest.assertEQ(MySpace.message, "Message has been reset",  "Message should be reset outside namespace")

let rectangleShape: MySpace.IShape = new MySpace.Rectangle(5, 10);
arktest.assertEQ(rectangleShape.area(), 50,  "Area of rectangleShape should be 50 outside namespace")

// Test code starts here
function main() {
    // Reset state before running tests to ensure no previous state affects the tests
    MySpace.resetState();

    // 1. Test variable assignment
    arktest.assertEQ(MySpace.count, 123,  "Count should be updated to 456")

    // 2. Test function call
    MySpace.resetMessage();
    arktest.assertEQ(MySpace.message, "Message has been reset",  "Message should be reset after function call")

    // 3. Test class instantiation and interface implementation
    let rectangle = new MySpace.Rectangle(5, 10);
    arktest.assertEQ(rectangle.area(), 50,  "Area of rectangle should be 50")
}
