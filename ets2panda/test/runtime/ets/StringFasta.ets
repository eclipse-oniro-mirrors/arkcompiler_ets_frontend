/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
    static class Random  {
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
            if (seqi + lenOut < seq.length as int) {
                ret += seq.substring(seqi, seqi + lenOut).length as int;
                seqi += lenOut;
            }
            else {
                let s : String = seq.substring(seqi);
                seqi = lenOut - s.length as int;
                ret += (s + seq.substring(0, seqi)).length as int;
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
            ret += new String(line).length as int;
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

        arktest.assertEQ(ret, this.expected,  "Incorrect result")
    }
}

function main(): void {
  let a = new StringFasta;
  a.run();
}
