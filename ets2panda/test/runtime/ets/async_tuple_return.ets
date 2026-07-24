/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

async function fetchPairAsync(): Promise<[number, string]> {
  return [1.0, "hello"]
}

async function fetchTripleAsync(): Promise<[number, string, boolean]> {
  return [42, "world", true]
}

async function fetchSingleAsync(): Promise<[number]> {
  return [100]
}

async function fetchNestedAsync(): Promise<[[number, string], boolean]> {
  return [[1, "a"], true]
}

async function fetchDifferentTypesAsync(): Promise<[string, number, boolean, string]> {
  return ["first", 2, false, "last"]
}

async function returnEmptyTuple(): Promise<[]> {
  return []
}

async function returnTupleWithArray(): Promise<[number, string[]]> {
  return [1, ["a", "b"]]
}

async function returnTupleWithNestedArray(): Promise<[number, [string, number]]> {
  return [1, ["a", 2]]
}

function main(): void {
  waitForCompletion(async () => {
    let pair = await fetchPairAsync()
    arktest.assertEQ(pair[0], 1.0)
    arktest.assertEQ(pair[1], "hello")

    let triple = await fetchTripleAsync()
    arktest.assertEQ(triple[0], 42)
    arktest.assertEQ(triple[1], "world")
    arktest.assertEQ(triple[2], true)

    let single = await fetchSingleAsync()
    arktest.assertEQ(single[0], 100)

    let nested = await fetchNestedAsync()
    arktest.assertEQ(nested[0][0], 1)
    arktest.assertEQ(nested[0][1], "a")
    arktest.assertEQ(nested[1], true)

    let diffTypes = await fetchDifferentTypesAsync()
    arktest.assertEQ(diffTypes[0], "first")
    arktest.assertEQ(diffTypes[1], 2)
    arktest.assertEQ(diffTypes[2], false)
    arktest.assertEQ(diffTypes[3], "last")

    let empty = await returnEmptyTuple()
    arktest.assertEQ(empty.length, 0)

    let withArray = await returnTupleWithArray()
    arktest.assertEQ(withArray[0], 1)
    arktest.assertEQ(withArray[1][0], "a")
    arktest.assertEQ(withArray[1][1], "b")

    let nestedArray = await returnTupleWithNestedArray()
    arktest.assertEQ(nestedArray[0], 1)
    arktest.assertEQ(nestedArray[1][0], "a")
    arktest.assertEQ(nestedArray[1][1], 2)
  })
}
