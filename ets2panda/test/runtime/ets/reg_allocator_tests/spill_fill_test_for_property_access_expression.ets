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
 
class C {
    x: int = 5;
}
class Sup {
    private _v: int = 0;

    get v(): int {
        return this._v;
    }

    set v(value: int) {
        this._v = value;
    }
}

final class Sub extends Sup {
    private _finalv: int = 0;
    get finalv(): int {
        return this._finalv;
    }
    set finalv(value: int) {
        this._finalv = value;
    }
}

function main(): void {
    let sup: Sup = new Sup();
    let sub: Sub = new Sub();
         let d0: number = 0, d1: number = 1, d2: number = 2, d3: number = 3, d4: number = 4, d5: number = 5,
 	         d6: number = 6, d7: number = 7, d8: number = 8, d9: number = 9,
 	         d10: number = 10, d11: number = 11, d12: number = 12, d13: number = 13, d14: number = 14, d15: number = 15,
 	         d16: number = 16, d17: number = 17, d18: number = 18, d19: number = 19,
 	         d20: number = 20, d21: number = 21, d22: number = 22, d23: number = 23, d24: number = 24, d25: number = 25,
 	         d26: number = 26, d27: number = 27, d28: number = 28, d29: number = 29,
 	         d30: number = 30, d31: number = 31, d32: number = 32, d33: number = 33, d34: number = 34, d35: number = 35,
 	         d36: number = 36, d37: number = 37, d38: number = 38, d39: number = 39,
 	         d40: number = 40, d41: number = 41, d42: number = 42, d43: number = 43, d44: number = 44, d45: number = 45,
 	         d46: number = 46, d47: number = 47, d48: number = 48, d49: number = 49,
 	         d50: number = 50, d51: number = 51, d52: number = 52, d53: number = 53, d54: number = 54, d55: number = 55,
 	         d56: number = 56, d57: number = 57, d58: number = 58, d59: number = 59,
 	         d60: number = 60, d61: number = 61, d62: number = 62, d63: number = 63, d64: number = 64, d65: number = 65,
 	         d66: number = 66, d67: number = 67, d68: number = 68, d69: number = 69,
 	         d70: number = 70, d71: number = 71, d72: number = 72, d73: number = 73, d74: number = 74, d75: number = 75,
 	         d76: number = 76, d77: number = 77, d78: number = 78, d79: number = 79,
 	         d80: number = 80, d81: number = 81, d82: number = 82, d83: number = 83, d84: number = 84, d85: number = 85,
 	         d86: number = 86, d87: number = 87, d88: number = 88, d89: number = 89,
 	         d90: number = 90, d91: number = 91, d92: number = 92, d93: number = 93, d94: number = 94, d95: number = 95,
 	         d96: number = 96, d97: number = 97, d98: number = 98, d99: number = 99,
 	         d100: number = 100, d101: number = 101, d102: number = 102, d103: number = 103, d104: number = 104, d105: number = 105,
 	         d106: number = 106, d107: number = 107, d108: number = 108, d109: number = 109,
 	         d110: number = 110, d111: number = 111, d112: number = 112, d113: number = 113, d114: number = 114, d115: number = 115,
 	         d116: number = 116, d117: number = 117, d118: number = 118, d119: number = 119,
 	         d120: number = 120, d121: number = 121, d122: number = 122, d123: number = 123, d124: number = 124, d125: number = 125,
 	         d126: number = 126, d127: number = 127, d128: number = 128, d129: number = 129,
 	         d130: number = 130, d131: number = 131, d132: number = 132, d133: number = 133, d134: number = 134, d135: number = 135,
 	         d136: number = 136, d137: number = 137, d138: number = 138, d139: number = 139,
 	         d140: number = 140, d141: number = 141, d142: number = 142, d143: number = 143, d144: number = 144, d145: number = 145,
 	         d146: number = 146, d147: number = 147, d148: number = 148, d149: number = 149,
 	         d150: number = 150, d151: number = 151, d152: number = 152, d153: number = 153, d154: number = 154, d155: number = 155,
 	         d156: number = 156, d157: number = 157, d158: number = 158, d159: number = 159,
 	         d160: number = 160, d161: number = 161, d162: number = 162, d163: number = 163, d164: number = 164, d165: number = 165,
 	         d166: number = 166, d167: number = 167, d168: number = 168, d169: number = 169,
 	         d170: number = 170, d171: number = 171, d172: number = 172, d173: number = 173, d174: number = 174, d175: number = 175,
 	         d176: number = 176, d177: number = 177, d178: number = 178, d179: number = 179,
 	         d180: number = 180, d181: number = 181, d182: number = 182, d183: number = 183, d184: number = 184, d185: number = 185,
 	         d186: number = 186, d187: number = 187, d188: number = 188, d189: number = 189,
 	         d190: number = 190, d191: number = 191, d192: number = 192, d193: number = 193, d194: number = 194, d195: number = 195,
 	         d196: number = 196, d197: number = 197, d198: number = 198, d199: number = 199,
 	         d200: number = 200, d201: number = 201, d202: number = 202, d203: number = 203, d204: number = 204, d205: number = 205,
 	         d206: number = 206, d207: number = 207, d208: number = 208, d209: number = 209,
 	         d210: number = 210, d211: number = 211, d212: number = 212, d213: number = 213, d214: number = 214, d215: number = 215,
 	         d216: number = 216, d217: number = 217, d218: number = 218, d219: number = 219,
 	         d220: number = 220, d221: number = 221, d222: number = 222, d223: number = 223, d224: number = 224, d225: number = 225,
 	         d226: number = 226, d227: number = 227, d228: number = 228, d229: number = 229,
 	         d230: number = 230, d231: number = 231, d232: number = 232, d233: number = 233, d234: number = 234, d235: number = 235,
 	         d236: number = 236, d237: number = 237, d238: number = 238, d239: number = 239,
 	         d240: number = 240, d241: number = 241, d242: number = 242, d243: number = 243, d244: number = 244, d245: number = 245,
 	         d246: number = 246, d247: number = 247, d248: number = 248, d249: number = 249,
 	         d250: number = 250, d251: number = 251;

    sup.v = 10;
    arktest.assertEQ(sup.v, 10);


    sub.finalv = 10;
    arktest.assertEQ(sub.finalv, 10);

}