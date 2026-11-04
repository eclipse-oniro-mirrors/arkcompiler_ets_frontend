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

export class StringBase64  {
    static readonly TO_BASE64_TABLE : String = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" + "abcdefghijklmnopqrstuvwxyz0123456789+/";
    static readonly BASE64PAD : char = c'=';
    static readonly TO_BINARY_TABLE : int[] = [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 0, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1];
     toBase64(data : String): String {
        let result : StringBuilder = new StringBuilder();
        let length : int = data.length.toInt();
        let i : int ;
        for (i = 0; i < (length - 2); i += 3) {
            result.append(StringBase64.TO_BASE64_TABLE.charAt(data.charAt(i).toInt() >> 2));
            result.append(StringBase64.TO_BASE64_TABLE.charAt(((data.charAt(i).toInt() & 0x03) << 4) + (data.charAt(i + 1).toInt() >> 4)));
            result.append(StringBase64.TO_BASE64_TABLE.charAt(((data.charAt(i + 1).toInt() & 0x0f) << 2) + (data.charAt(i + 2).toInt() >> 6)));
            result.append(StringBase64.TO_BASE64_TABLE.charAt(data.charAt(i + 2).toInt() & 0x3f));
        }
        if (length % 3 != 0) {
            i = length - (length % 3);
            result.append(StringBase64.TO_BASE64_TABLE.charAt(data.charAt(i).toInt() >> 2));
            if ((length % 3) == 2) {
                result.append(StringBase64.TO_BASE64_TABLE.charAt(((data.charAt(i).toInt() & 0x03) << 4) + (data.charAt(i + 1).toInt() >> 4)));
                result.append(StringBase64.TO_BASE64_TABLE.charAt((data.charAt(i + 1).toInt() & 0x0f) << 2));
                result.append(StringBase64.BASE64PAD);
            }
            else {
                result.append(StringBase64.TO_BASE64_TABLE.charAt((data.charAt(i).toInt() & 0x03) << 4));
                result.append(StringBase64.BASE64PAD);
                result.append(StringBase64.BASE64PAD);
            }
        }
        return result.toString();
    }
     base64ToString(data : String): String {
        let result : StringBuilder = new StringBuilder();
        let leftbits : int = 0;
        let leftdata : int = 0;
        for (let i : int = 0; i < data.length; i++) {
            let c : int = StringBase64.TO_BINARY_TABLE[data.charAt(i).toInt() & 0x7f];
            let padding : boolean = data.charAt(i) == StringBase64.BASE64PAD;
            if (c == -1) {
                continue;
            }
            leftdata = (leftdata << 6) | c;
            leftbits += 6;
            if (leftbits >= 8) {
                leftbits -= 8;
                if (!padding) {
                    result.append(((leftdata >> leftbits) & 0xff).toChar());
                }
                leftdata &= (1 << leftbits) - 1;
            }
        }
        if (leftbits != 0) {
        }
        return result.toString();
    }
    n1 : int = 8192;
    n2 : int = 16384;
    public  run(): void {
        let str : String = "";
        for (let i : int = 0; i < this.n1; i++) {
            str += ((25 * random().toInt()) + 97).toChar();
        }
        for (let i : int = this.n1; i <= this.n2; i *= 2) {
            let base64 : String = this.toBase64(str);
            let encoded : String = this.base64ToString(base64);
            if (!encoded.equals(str)) {
            }
            str += str;
        }
//        Consumer.consumeObj(str);
    }
}

function main(): void {
  let a = new StringBase64;
  a.run();
}
