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
 
class Overload {
    public static func(): string {
        return "zero";
    }

    public static func(a: number): string {
        return "one";
    }

    public static func(a: number, b: number): string {
        return "two";
    }

    public static func(...args: number[]): string {
        return "variadic";
    }
}

function main(): void {
    arktest.assertEQ(Overload.func(), "zero")
    arktest.assertEQ(Overload.func(1), "one")
    arktest.assertEQ(Overload.func(1, 2), "two")
    arktest.assertEQ(Overload.func(1, 2, 3), "variadic")

    let arr: number[] = [1, 2];
    arktest.assertEQ(Overload.func(...arr), "variadic")
}
