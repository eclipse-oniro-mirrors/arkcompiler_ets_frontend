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
interface ConvertibleTo<T> {
  convert(): T;
}

interface TwoWayConvertible<T extends ConvertibleTo<S>, S extends ConvertibleTo<T>> {
  t2s(s: S): T;
  s2t(t: T): S;
}

class Incremented implements ConvertibleTo<Decremented> {
  i: number;
  constructor(i: number) {
    this.i = ++i;
  }
  convert(): Decremented {
    return new Decremented(this.i - 1);
  }
}

class Decremented implements ConvertibleTo<Incremented> {
  i: number;
  constructor(i: number) {
    this.i = i - 1;
  }
  convert(): Incremented {
    return new Incremented(this.i + 1);
  }
}

class Intf5901 implements TwoWayConvertible<Incremented, Decremented> {
  public t2s(d: Decremented): Incremented {
    return d.convert();
  }

  public s2t(i: Incremented): Decremented {
    return i.convert();
  }
}

function testIntf5901() {
  const converter = new Intf5901();
  const value = 5;
  const d = new Decremented(value);
  const i = converter.t2s(d);
  arktest.assertEQ(i.i, value + 1);
  const d2 = converter.s2t(i);
  arktest.assertEQ(d2.i, value - 1);
  arktest.assertEQ(d2.i, d.i);
  
  const chainValue = 20;
  const start = new Incremented(chainValue);
  const middle = converter.s2t(start);
  arktest.assertEQ(middle.i, chainValue - 1);
  const end = converter.t2s(middle);
  arktest.assertEQ(end.i, chainValue + 1);
  arktest.assertEQ(end.i, start.i);
}

testIntf5901();
