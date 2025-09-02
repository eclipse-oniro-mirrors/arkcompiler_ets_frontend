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

type T0 = Object|null|undefined
type F1<T> = (a:T, b:T) => void
type F2<T> = null|undefined|(a:T, b:T) => void

function equ<T>(a: T, b:T){
  return a == b
}

function equs<T>(a: T, b:T){
  return a === b
}

function nequ<T>(a: T, b:T){
  return a != b
}

function nequs<T>(a: T, b:T){
  return a !== b
}

class A {}

function main() {
  arktest.assertEQ(equ<T0>(null, undefined), true)
  arktest.assertEQ(equs<T0>(null, undefined), false)

  arktest.assertEQ(nequ<T0>(null, undefined), false)
  arktest.assertEQ(nequs<T0>(null, undefined), true)

  arktest.assertTrue(null == undefined)
  arktest.assertNE(null, undefined)

  arktest.assertTrue(!(null != undefined))
  arktest.assertTrue(null !== undefined)

  let f1 = (a:Int, b:Int) => { a=a-b; }
  let f2 = (a:Int, b:Int) => { a=a+b; }

  arktest.assertEQ(equ<F1<Int>>(f1, f1), true)
  arktest.assertEQ(equ<F1<Int>>(f1, f2), false)
  arktest.assertEQ(equs<F1<Int>>(f1, f1), true)
  arktest.assertEQ(equs<F1<Int>>(f1, f2), false)

  let a1 = new A()
  let a2 = new A()

  arktest.assertEQ(equs<T0>(a1, a1), true)
  arktest.assertEQ(nequs<T0>(a1, a1), false)
  arktest.assertEQ(equs<T0>(a1, a2), false)
  arktest.assertEQ(nequs<T0>(a1, a2), true)

  arktest.assertTrue(10 === 10)
}
