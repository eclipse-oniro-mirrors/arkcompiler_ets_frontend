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

export class StringFasta  {
    static readonly ALU : String = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" + "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" + "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" + "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" + "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" + "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" + "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
    static IUB : HashMap<Char, Double> = new HashMap();
    static HomoSap : HashMap<Char, Double> = new HashMap();
    static {
        IUB.put(c'a', 0.2);
        IUB.put(c'c', 0.2);
        IUB.put(c'g', 0.2);
        IUB.put(c't', 0.2);
        IUB.put(c'B', 0.2);
        IUB.put(c'D', 0.2);
        IUB.put(c'H', 0.2);
        IUB.put(c'K', 0.2);
        IUB.put(c'M', 0.2);
        IUB.put(c'N', 0.2);
        IUB.put(c'R', 0.2);
        IUB.put(c'S', 0.2);
        IUB.put(c'V', 0.2);
        IUB.put(c'W', 0.2);
        IUB.put(c'Y', 0.2);
        HomoSap.put(c'a', 0.3029549426680);
        HomoSap.put(c'c', 0.1979883004921);
        HomoSap.put(c'g', 0.1975473066391);
        HomoSap.put(c't', 0.3015094502008);
    }
    static /* @@ label */class Random  {
        static last : int = 42;
        static A : int = 3877;
        static C : int = 29573;
        static M : int = 139968;
        public static rand(max : double): double {
            last = (last * A + C) % M;
            return max * last / M;
        }
    }

    static makeCumulative(table : HashMap<Char, Double>): void {
        let last : Char = null;
        for (let entry : HashMap.Entry<Char, Double> of table.entrySet()){
            let c : Char = entry.getKey();
            if (last != null) {
                table.put(c, entry.getValue() + table.get(last));
            }
            last = c;
        }
    }
    static fastaRepeat(n : int, seq : String): int {
        let seqi : int = 0;
        let lenOut : int = 60;
        let ret : int = 0;
        while(n > 0)
        {
            if (n < lenOut) {
                lenOut = n;
            }
            if (seqi + lenOut < seq.length()) {
                ret += seq.substring(seqi, seqi + lenOut).length();
                seqi += lenOut;
            }
            else {
                let s : String = seq.substring(seqi);
                seqi = lenOut - s.length();
                ret += (s + seq.substring(0, seqi)).length();
            }
            n -= lenOut;
        }
        return ret;
    }
    static fastaRandom(n : int, table : HashMap<Char, Double>): int {
        let line : char[] = new char[60];
        makeCumulative(table);
        let ret : int = 0;
        while(n > 0)
        {
            if (n < line.length) {
                line = new char[n];
            }
            for (let i : int = 0; i < line.length; i++) {
                let r : double = Random.rand(1);
                for (let entry : HashMap.Entry<Char, Double> of table.entrySet()){
                    let c : Char = entry.getKey();
                    if (r < entry.getValue()) {
                        line[i] = c;
                        break;
                    }
                }
            }
            ret += new String(line).length();
            n -= line.length;
        }
        return ret;
    }
    count : int = 7;
    static readonly expected : int = 1456000;
    public  run(): void {
        let ret : int = 0;
        ret += fastaRepeat(2 * count * 100000, ALU);
        ret += fastaRandom(3 * count * 1000, IUB);
        ret += fastaRandom(5 * count * 1000, HomoSap);

        arktest.assertEQ(ret, this.expected, "Incorrect result");
    }
}

function main(): void {
  let a = new StringFasta;
  a.run();
}

/* @@? 18:18 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 18:46 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 19:22 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 19:50 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 21:9 Error TypeError: Unresolved reference IUB */
/* @@? 22:9 Error TypeError: Unresolved reference IUB */
/* @@? 23:9 Error TypeError: Unresolved reference IUB */
/* @@? 24:9 Error TypeError: Unresolved reference IUB */
/* @@? 25:9 Error TypeError: Unresolved reference IUB */
/* @@? 26:9 Error TypeError: Unresolved reference IUB */
/* @@? 27:9 Error TypeError: Unresolved reference IUB */
/* @@? 28:9 Error TypeError: Unresolved reference IUB */
/* @@? 29:9 Error TypeError: Unresolved reference IUB */
/* @@? 30:9 Error TypeError: Unresolved reference IUB */
/* @@? 31:9 Error TypeError: Unresolved reference IUB */
/* @@? 32:9 Error TypeError: Unresolved reference IUB */
/* @@? 33:9 Error TypeError: Unresolved reference IUB */
/* @@? 34:9 Error TypeError: Unresolved reference IUB */
/* @@? 35:9 Error TypeError: Unresolved reference IUB */
/* @@? 36:9 Error TypeError: Unresolved reference HomoSap */
/* @@? 37:9 Error TypeError: Unresolved reference HomoSap */
/* @@? 38:9 Error TypeError: Unresolved reference HomoSap */
/* @@? 39:9 Error TypeError: Unresolved reference HomoSap */
/* @@@ label Error SyntaxError: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 47:13 Error TypeError: Static property 'last' must be accessed through it's class 'Random' */
/* @@? 47:21 Error TypeError: Static property 'last' must be accessed through it's class 'Random' */
/* @@? 47:28 Error TypeError: Static property 'A' must be accessed through it's class 'Random' */
/* @@? 47:32 Error TypeError: Static property 'C' must be accessed through it's class 'Random' */
/* @@? 47:37 Error TypeError: Static property 'M' must be accessed through it's class 'Random' */
/* @@? 48:26 Error TypeError: Static property 'last' must be accessed through it's class 'Random' */
/* @@? 48:33 Error TypeError: Static property 'M' must be accessed through it's class 'Random' */
/* @@? 52:35 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 53:27 Error TypeError: Type 'null' cannot be assigned to type 'Char' */
/* @@? 54:26 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 54:57 Error TypeError: 'For-of' statement source expression is not of iterable type. */
/* @@? 71:33 Error TypeError: Type 'Int' has no call signatures. */
/* @@? 72:24 Error TypeError: Type 'Int' has no call signatures. */
/* @@? 77:33 Error TypeError: Type 'Int' has no call signatures. */
/* @@? 78:24 Error TypeError: Type 'Int' has no call signatures. */
/* @@? 84:41 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 86:9 Error TypeError: Static property 'makeCumulative' must be accessed through it's class 'StringFasta' */
/* @@? 94:34 Error TypeError: Static property 'Random' must be accessed through it's class 'StringFasta' */
/* @@? 95:34 Error TypeError: Cannot find type 'HashMap'. */
/* @@? 95:65 Error TypeError: 'For-of' statement source expression is not of iterable type. */
/* @@? 103:20 Error TypeError: Type 'Int' has no call signatures. */
/* @@? 112:16 Error TypeError: Static property 'fastaRepeat' must be accessed through it's class 'StringFasta' */
/* @@? 112:32 Error TypeError: Property 'count' must be accessed through 'this' */
/* @@? 113:16 Error TypeError: Static property 'fastaRandom' must be accessed through it's class 'StringFasta' */
/* @@? 113:32 Error TypeError: Property 'count' must be accessed through 'this' */
/* @@? 113:46 Error TypeError: Unresolved reference IUB */
/* @@? 114:16 Error TypeError: Static property 'fastaRandom' must be accessed through it's class 'StringFasta' */
/* @@? 114:32 Error TypeError: Property 'count' must be accessed through 'this' */
/* @@? 114:46 Error TypeError: Unresolved reference HomoSap */
/* @@? 116:36 Error TypeError: 'expected' is a static property of 'StringFasta' */
