/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import { p, foo, pFuncCall, arrowFunc, pArrowCall } from "./no_await_js_promise_export";

async function awaitPromise() {
  return await p;
}

async function awaitFunctionCall() {
  return await foo();
}

async function awaitFuncResult() {
  return await pFuncCall;
}

async function awaitArrowCall() {
  return await arrowFunc();
}

async function awaitArrowResult() {
  return await pArrowCall;
}

class ExampleClass {
  async classMethod() {
    return await p;
  }

  handler = async () => {
    return await pFuncCall;
  };
}

const exampleObj = {
  async objMethod() {
    return await pArrowCall;
  },

  arrowHandler: async () => {
    return await foo();
  }
};

(async function() {
  console.log("IIFE result:", await p);
})();

(async () => {
  console.log("IIFE Arrow result:", await arrowFunc());
})();