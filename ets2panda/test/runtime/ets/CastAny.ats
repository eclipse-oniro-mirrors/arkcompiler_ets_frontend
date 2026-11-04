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

let v: Any = undefined

function anyAsString(a: Any) {
  v = a as string
}

function anyAsStringOrUndefined(a: Any) {
  v = a as (string | undefined)
}

function anyAsNumber(a: Any) {
  v = a as number
}

function anyAsNull(a: Any) {
  v = a as null
}

function anyAsObject(a: Any) {
  v = a as object
}

function main() {
  let caught = false;
  try {
    // Instruction 'checkcast' is generated and it doesn't throw an exception here.
    anyAsStringOrUndefined(undefined);
  } catch (e) {
    if (e instanceof ClassCastError) {
      caught = true
    }
  }
  arktest.assertFalse(caught)

  arktest.expectThrow(() => {
    // Instruction 'checkcast.nonnull' is generated and it throws an exception here.
    anyAsString(undefined);
  }, (e: Error) => {
    return e instanceof ClassCastError;
  });

  caught = false;
  try {
    // Instruction 'checkcast.nonnull' is generated and it doesn't throw an exception here.
    anyAsString('test');
  } catch (e) {
    if (e instanceof ClassCastError) {
      caught = true
    }
  }
  arktest.assertFalse(caught)

  arktest.expectThrow(() => {
    // Instruction 'checkcast.nonnull' is generated and it throws an exception here.
    anyAsNumber(undefined);
  }, (e: Error) => {
    return e instanceof ClassCastError;
  });

  arktest.expectThrow(() => {
    // Instruction 'checkcast.nonnull' is generated and it throws an exception here.
    anyAsNull(undefined);
  }, (e: Error) => {
    return e instanceof ClassCastError;
  });

  arktest.expectThrow(() => {
    // Instruction 'checkcast.nonnull' is generated and it throws an exception here.
    anyAsObject(undefined);
  }, (e: Error) => {
    return e instanceof ClassCastError;
  });
}
