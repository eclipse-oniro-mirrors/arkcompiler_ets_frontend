/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

namespace NS {
  export class C {
    public x: number

    public constructor(){
        this.x = 1
    }

    public static $_invoke(): C {
      return new C();
    }
  }
}

export class C {
    public x: number

    public constructor(){
        this.x = 2
    }

  public static $_invoke(): C {
    return new C();
  }
}

function main(): void {
  let c: NS.C = NS.C();
  let d: NS.C = NS.C.$_invoke();
  let e: NS.C = new NS.C();

  let f: C = C();
  let g: C = C.$_invoke();
  let h: C = new C();

  arktest.assertEQ(c.x, 1)
  arktest.assertEQ(d.x, 1)
  arktest.assertEQ(e.x, 1)

  arktest.assertEQ(f.x, 2)
  arktest.assertEQ(g.x, 2)
  arktest.assertEQ(h.x, 2)
}
