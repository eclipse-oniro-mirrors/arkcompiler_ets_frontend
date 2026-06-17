/*
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

function expectClassCastError(fn: () => void): void {
    arktest.expectThrow(fn, (e: Error) => { return e instanceof ClassCastError })
}

function expectCastToCharFails<T>(name: string, value: T, ts: arktest.ArkTestsuite): void {
    ts.addTest(name, (): void => {
        expectClassCastError((): void => { value as Char })
    })
}

function main() {
    let ts = new arktest.ArkTestsuite("cast_warning_runtime_class_cast_error")

    expectCastToCharFails("Byte to Char warning cast throws ClassCastError", new Byte(1 as byte), ts)
    expectCastToCharFails("Short to Char warning cast throws ClassCastError", new Short(2 as short), ts)
    expectCastToCharFails("Int to Char warning cast throws ClassCastError", new Int(3 as int), ts)
    expectCastToCharFails("Long to Char warning cast throws ClassCastError", new Long(4 as long), ts)
    expectCastToCharFails("Float to Char warning cast throws ClassCastError", new Float(5 as float), ts)
    expectCastToCharFails("Double to Char warning cast throws ClassCastError", new Double(6 as double), ts)

    ts.addTest("Char to Byte warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as Byte })
    })

    ts.addTest("Char to Short warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as Short })
    })

    ts.addTest("Char to Int warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as Int })
    })

    ts.addTest("Char to Long warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as Long })
    })

    ts.addTest("Char to Float warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as Float })
    })

    ts.addTest("Char to Double warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as Double })
    })

    ts.addTest("Char to String warning cast throws ClassCastError", (): void => {
        let value: Char = new Char(c'A')
        expectClassCastError((): void => { value as String })
    })

    return ts.run()
}
