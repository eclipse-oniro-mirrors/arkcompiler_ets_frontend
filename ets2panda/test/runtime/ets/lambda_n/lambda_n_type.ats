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

// function foo(f: (a: number) => number) { return f(1) }
// foo((a: number) => a)

function foo(f: (s1: String, s2: String, s3: String, s4: String, s5: String,
                 s6: String, s7: String, s8: String, s9: String, s10: String,
                 s11: String, s12: String, s13: String, s14: String, s15: String,
                 s16: String, s17: String) => String) {
                    return f("1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
                             "12", "13", "14", "15", "16", "17");
                 }

type fn_type = (s1: String, s2: String, s3: String, s4: String, s5: String,
                    s6: String, s7: String, s8: String, s9: String, s10: String,
                    s11: String, s12: String, s13: String, s14: String, s15: String,
                    s16: String, s17: String) => void;

function main(): void {

    let fn: fn_type = (str1: String, str2: String, str3: String): void => {
        arktest.assertEQ(str1 + str2 + str3, "helloworld!");
    };
    fn("hello", "world", "!");

    let fooRetVal =
        foo((s1: String, s2: String, s3: String, s4: String, s5: String,
            s6: String, s7: String, s8: String, s9: String, s10: String,
            s11: String, s12: String, s13: String, s14: String, s15: String,
            s16: String, s17: String): String => {
                return s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10 + s11 + s12 + s13 + s14 + s15 + s16 + s17;
            });
    arktest.assertEQ(fooRetVal, "1234567891011121314151617")
}
