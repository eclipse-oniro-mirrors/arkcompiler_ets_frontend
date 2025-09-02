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

class A { name = "LambdaNTest: " }
let a = new A();
let lambdaNWithReceiver = (
    this: A,
    str1: String,
    str2: String,
    str3: String,
    str4: String,
    str5: String,
    str6: String,
    str7: String,
    str8: String,
    str9: String,
    str10: String,
    str11: String,
    str12: String,
    str13: String,
    str14: String,
    str15: String,
    str16: String,
    str17: String,
    str18: String
): String => {
    let res: String = str1 + str2 + str3 + str4 + str5 + str6 + str7 + str8 + str9 + str10 + str11 + str12 + str13 + str14 + str15 + str16 + str17 + str18;
    return this.name + res;
}

arktest.assertEQ(lambdaNWithReceiver(a, "hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!")
arktest.assertEQ(a.lambdaNWithReceiver("hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!")
arktest.assertEQ(new A().lambdaNWithReceiver("hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!")
arktest.assertEQ(lambdaNWithReceiver(new A(), "hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!")

let lambdaNWithRestAndReceiver = (
    this: A,
    str1: String,
    str2: String,
    str3: String,
    str4: String,
    str5: String,
    str6: String,
    str7: String,
    str8: String,
    str9: String,
    str10: String,
    str11: String,
    str12: String,
    str13: String,
    str14: String,
    str15: String,
    str16: String,
    str17: String,
    ...restStr: FixedArray<String>
): String => {
    let res: String = str1 + str2 + str3 + str4 + str5 + str6 + str7 + str8 + str9 + str10 + str11 + str12 + str13 + str14 + str15 + str16 + str17;
    for (let s of restStr) {
        res += s;
    }
    return this.name + res;
}

arktest.assertEQ(lambdaNWithRestAndReceiver(a, "hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!!!")
arktest.assertEQ(a.lambdaNWithRestAndReceiver("hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!!!")
arktest.assertEQ(new A().lambdaNWithRestAndReceiver("hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!!!")
arktest.assertEQ(lambdaNWithRestAndReceiver(new A(), "hello", "world", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!", "!"), "LambdaNTest: helloworld!!!!!!!!!!!!!!!!!!")