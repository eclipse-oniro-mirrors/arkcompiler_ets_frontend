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

class A {}

function main(){
   let $$a = new A();
   let b = $$a;
   arktest.assertNE(b, null)

   let c$$ = true;
   let d = c$$;
   arktest.assertEQ(d, true)

   let $$e$$ = 1;
   let f = $$e$$;
   arktest.assertEQ(f, 1)

   let $$g$ = false;
   let h = $$g$;
   arktest.assertEQ(h, false)

   let $i$$ = false;
   let j = $i$$;
   arktest.assertEQ(j, false)

   let $k$ = "apple";
   let m = $k$;
   arktest.assertEQ(m, "apple")

}