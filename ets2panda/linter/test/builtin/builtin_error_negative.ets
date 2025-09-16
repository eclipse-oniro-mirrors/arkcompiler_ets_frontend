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

let err1: AggregateError = new AggregateError([1]); //NoPropertyDescriptor
let err2: AggregateError = new AggregateError([1]); //NoPropertyDescriptor
let errors: Object[] = err2.errors; // NoPropertyDescriptor
try {
  throw new Error(); //lib.es2022.error.d.ts
} catch (e) {
  new Error("Connecting to database failed.", {cause: e});
}
function get(err: AggregateError):Error { //NoPropertyDescriptor
  const a = AggregateError.name;
  const errr = Error() //BuiltinNoCtorFunc
  return new Error; //lib.es2022.error.d.ts
}
const err3 = new AggregateError([1, "two", new Error("fail")]); //NoPropertyDescriptor
console.log(err3.errors.toString()); // NoPropertyDescriptor
console.log(err3.message); // NoPropertyDescriptor?

const err4 = new AggregateError( //NoPropertyDescriptor
  [new TypeError("invalid type"), new RangeError("out of bounds")],
  "Multiple errors occurred"
);
console.log(err4.errors.length.toString()); // NoPropertyDescriptor + BuiltinAll

function fetchData() {
  try {
    throw new Error("Network timeout"); //BuiltinNoCtorFunc lib.es2022.error.d.ts
  } catch (e) {
    return new Error("Fetch failed", { cause: e });
  }
}

const result = fetchData();
if (result.cause) {//NoPropertyDescriptor
  console.log("Root cause:", result.cause.message); // NoPropertyDescriptor
}
console.log(err4.errors.toString()); // NoPropertyDescriptor

//sum:18-1