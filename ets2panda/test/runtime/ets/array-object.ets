/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
  let arr1: FixedArray<Int> = [1,2,3,4,5];
  let arr2: FixedArray<int> = [1,2,3,4,5];
  let object_array: FixedArray<Object> = arr1 as FixedArray<Object>;
  let obj: Object = arr1 as Object;

// TypeError: Cannot access property of non-object or non-enum type
// arr1.toString();
// arr2.toString();
// object_array.toString();

  obj.toString();

//  TypeError: Cannot access property of non-object or non-enum type
//  arr1.$_hashCode();
//  arr2.$_hashCode();
//  object_array.$_hashCode();

  obj.$_hashCode();

  arktest.assertEQ(arr1, arr1)
  arktest.assertEQ(arr1, object_array)
  arktest.assertEQ(arr1, obj)
  arktest.assertNE(arr1, arr2)

// Cannot cast type 'int[]' to 'Object[]'
//  let object_array2: Object[] = arr2 as Object[];
//  arktest.assertEQ(obj.equals(arr2 as Object), false)

  arktest.assertEQ(arr2, arr2)
  arktest.assertNE(arr2, new int[5])
}
