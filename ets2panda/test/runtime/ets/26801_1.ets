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


let x=0, staticX=0, y=0, staticY=0;
class C {
  set x(v:int) { x = v; }
  static set staticX(v:int) { staticX = v; }
  set y(v:int) { y = v; }
  static set staticY(v:int) { staticY = v; }
}

function main(): void {
  arktest.assertEQ(new C().x = 1, 1, "`new C().x = 1` is `1`");
  arktest.assertEQ(x, 1, "The value of `x` is `1`");
  arktest.assertEQ(C.staticX = 2, 2, "`C.staticX = 2` is `2`");
  arktest.assertEQ(staticX, 2, "The value of `staticX` is `2`");
  arktest.assertEQ(new C().y = 3, 3, "`new C().y = 3` is `3`");
  arktest.assertEQ(y, 3, "The value of `y` is `3`");
  arktest.assertEQ(C.staticY = 4, 4, "`C.staticY = 4` is `4`");
  arktest.assertEQ(staticY, 4, "The value of `staticY` is `4`");
}
