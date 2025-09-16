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
    let a: double = 10.0;
    let b = -a;
    arktest.assertEQ(b, -10.0)

    a = -a;
    arktest.assertEQ(a, -10.0)

    let c = +a;
    arktest.assertEQ(c, -10.0)
    arktest.assertEQ(+c, -10.0)
    arktest.assertEQ(-c, 10.0)
  }

  {
    let a: int = 20;
    let b = -a;
    arktest.assertEQ(b, -20)

    a = -a;
    arktest.assertEQ(a, -20)

    let c = +a;
    arktest.assertEQ(c, -20)
    arktest.assertEQ(+c, -20)
    arktest.assertEQ(-c, 20)
  }

  {
    let a: Double = new Double(30.0);
    let b = -a;
    arktest.assertEQ(b, -30.0)

    a = -a;
    arktest.assertEQ(a, -30.0)
    arktest.assertEQ(a.toDouble(), -30.0)

    let c = +a;
    arktest.assertEQ(c, -30.0)
    arktest.assertEQ(+c, -30.0)
    arktest.assertEQ(-c, 30.0)
  }

  {
    let a: Int = new Int(40);
    let b = -a;
    arktest.assertEQ(b, -40)

    a = -a;
    arktest.assertEQ(a, -40)
    arktest.assertEQ(a.toDouble(), -40)

    let c = +a;
    arktest.assertEQ(c, -40)
    arktest.assertEQ(+c, -40)
    arktest.assertEQ(-c, 40)
  }

  {
    let a = -new Int(50);
    arktest.assertEQ(a, -50)

    let b = -a;
    arktest.assertEQ(b, 50)

    let c = +a;
    arktest.assertEQ(c, -50)
    arktest.assertEQ(+c, -50)
    arktest.assertEQ(-c, 50)
  }

  {
    let a = +new Double(60.0);
    arktest.assertEQ(a, 60.0)

    let b = -a;
    arktest.assertEQ(b, -60.0)

    let c = +a;
    arktest.assertEQ(c, 60.0)
    arktest.assertEQ(+c, 60.0)
    arktest.assertEQ(-c, -60.0)
  }

  {
    let a = -(-1);
    arktest.assertEQ(a, 1);

    let b = -(1);
    arktest.assertEQ(b, -1);

    let c = - -1;
    arktest.assertEQ(c, 1);

    let d = +(1);
    arktest.assertEQ(d, 1);

    let e = +(-1);
    arktest.assertEQ(e, -1);

    let f = + -1;
    arktest.assertEQ(f, -1);
  }
}
