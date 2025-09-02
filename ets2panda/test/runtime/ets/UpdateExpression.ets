/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function main(): void {
  {
    let a: byte = 10;
    let b = ++a;
    arktest.assertEQ(a, 11)
    arktest.assertEQ(b, 11)

    arktest.assertEQ(++a, 12)
    arktest.assertEQ(a, 12)
    arktest.assertEQ(b, 11)
  }

  {
    let a: int = 20;
    let b = a++;
    arktest.assertEQ(a, 21)
    arktest.assertEQ(b, 20)

    arktest.assertEQ(a++, 21)
    arktest.assertEQ(a, 22)
    arktest.assertEQ(b, 20)
  }

  {
    let a: Double = new Double(30.0);
    let b = ++a;
    arktest.assertEQ(a, 31.0)
    arktest.assertEQ(a.toDouble(), 31.0)
    arktest.assertEQ(b, 31.0)
    arktest.assertEQ(b.toDouble(), 31.0)

    arktest.assertEQ((++a).toDouble(), 32.0)
    arktest.assertEQ(a, 32.0)
    arktest.assertEQ(a.toDouble(), 32.0)
    arktest.assertEQ(b, 31.0)
    arktest.assertEQ(b.toDouble(), 31.0)
  }

  {
    let a: Int = new Int(40);
    let b = a++;
    arktest.assertEQ(a, 41)
    arktest.assertEQ(a.toInt(), 41)
    arktest.assertEQ(b, 40)
    arktest.assertEQ(b.toInt(), 40)

    arktest.assertEQ((a++).toInt(), 41)
    arktest.assertEQ(a, 42)
    arktest.assertEQ(a.toInt(), 42)
    arktest.assertEQ(b, 40)
    arktest.assertEQ(b.toInt(), 40)
  }

  {
    let fn: (x: Int) => Int = (x: Int): Int => { return x; };
    let a: Int = new Int(50);
    let b = fn(a++);
    arktest.assertEQ(a, 51)
    arktest.assertEQ(a.toInt(), 51)
    arktest.assertEQ(b, 50)
    arktest.assertEQ(b.toInt(), 50)

    arktest.assertEQ(fn(++a), 52)
    arktest.assertEQ(a, 52)
  }

  {
    let b: byte = 127;
    arktest.assertEQ(Byte.toInt(++b), -128)
    arktest.assertEQ(Byte.toInt(--b), 127)

    let c: char = 65535;
    arktest.assertEQ(Char.toInt(++c), 0)
    arktest.assertEQ(Char.toInt(--c), 65535)

    let s: short = 32767;
    arktest.assertEQ(Short.toInt(++s), -32768)
    arktest.assertEQ(Short.toInt(--s), 32767)
  }

  {
    let b: Int[] = [1, 2]
    arktest.assertEQ(++b[1], 3)
    arktest.assertEQ(--b[1], 2)
  }
}
