/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

import { BusinessError } from '@ohos.base';
import { AError, AError1, BError, BError1 } from "./ts-like-catch-type111";

try {
  throw new Error();
} catch(e) {
  e.message;
  e.prop;
}

try {
  throw new Error();
} catch(e) {
}

try {
  let that = 123;
} catch (err: BusinessError) {
  console.log(err.toString());
}

class RpcException {
  msg: string;
  code: number;
}

class RpcException1 {
  msg: string;
  code: number;
}

// not extends Error
function a1() {
  try {
    throw new RpcException('msg', 1);
  } catch (e) {
    let errorCode = 1;
    let errorMsg = '';
    if (e instanceof RpcException) {  // error
      errorMsg = e.msg; //  not check
      errorCode = e.code; //  not check
    } else if (e instanceof RpcException1) { // error
      errorMsg = e.msg; //  not check
      errorCode = e.code; //  not check
    } else {
      console.log('else prop: ' + e.prop); // error
      console.log('else message: ' + e.message); // ok
    }
    // e instanceof RpcException / e.prop error
    e instanceof RpcException ? e.msg : e.prop;
    // e instanceof RpcException / e.prop error
    e instanceof RpcException ? console.log('when true: ' + e.msg) : console.log('when false: ' + e.prop);
    // e instanceof RpcException / e instanceof RpcException1 / e.prop error
    e instanceof RpcException ? console.log('when true: ' + (e instanceof RpcException1 ? e.msg : e.code)) : console.log('when false: '  + (e instanceof RpcException1 ? e.msg : e.prop));
    //  e instanceof RpcException / e instanceof RpcException1
    e instanceof RpcException ? console.log('when true: ' + (e instanceof RpcException1 ? e.msg : e.code)) : console.log('when false: '  + (e instanceof RpcException1 ? e.msg : e.message)); // error * 2
    console.log(errorMsg);
    (e as RpcException).msg; // error
    console.log('errorMsg: ' + (e as RpcException).msg); // error
    const temp = e;
    temp.prop; // error
    console.log('errorMsg: ' + temp.prop);  // error
    const temp1 = e as RpcException; // error
    temp1.msg; // not check
    temp1.message; // not check
    console.log('errorMsg: ' + temp1.message); // not check
  }
}

// import / not extends Error
function a2() {
  try {
    throw new BError('msg', 1);
  } catch (e) {
    let errorCode = 1;
    let errorMsg = '';
    if (e instanceof BError) {  // error
      errorMsg = e.a; //  not check
      errorCode = e.code; //  not check
    } else if (e instanceof BError1) { // error
      errorMsg = e.b; //  not check
      errorCode = e.message; //  not check
    } else {
      console.log('else prop: ' + e.prop); // error
      console.log('else message: ' + e.message); // ok
    }
    // e instanceof BError / e.prop error
    e instanceof BError ? e.msg : e.prop;
    // e instanceof BError / e.prop error
    e instanceof BError ? console.log('when true: ' + e.msg) : console.log('when false: ' + e.prop);
    // e instanceof BError / e instanceof BError1 / e.prop error
    e instanceof BError ? console.log('when true: ' + (e instanceof BError1 ? e.msg : e.code)) : console.log('when false: '  + (e instanceof BError1 ? e.msg : e.prop));
    // e instanceof BError / e instanceof BError1
    e instanceof BError ? console.log('when true: ' + (e instanceof BError1 ? e.msg : e.code)) : console.log('when false: '  + (e instanceof BError1 ? e.msg : e.message));
    console.log(errorMsg);
    (e as BError).msg; // error
    console.log('errorMsg: ' + (e as BError).msg); // error
    const temp = e;
    temp.prop; // error
    console.log('errorMsg: ' + temp.prop);  // error
    const temp1 = e as BError; // error
    temp1.msg; // not check
    temp1.message; // not check
    console.log('errorMsg: ' + temp1.message); // not check
  }
}

class RpcException2 extends Error {
  msg: string;
  code: number;
  constructor(msg: string, code: number) {
    super(msg);
    this.msg = msg;
    this.code = code;
  }
}

class RpcException3 extends Error {
  msg: string;
  code: number;
  constructor(msg: string, code: number) {
    super(msg);
    this.msg = msg;
    this.code = code;
  }
}

// extends Error
function a3() {
  try {
    throw new RpcException2();
  } catch (e) {
    let errorCode = 1;
    let errorMsg = '';
    if (e instanceof RpcException2) {
      errorMsg = e.msg;
      errorCode = e.code;
      console.log('a3: ' + e.prop); // error
    } else if (e instanceof RpcException3) {
      errorMsg = e.msg;
      errorCode = e.code;
    } else {
      console.log('else prop: ' + e.prop); // error
      console.log('else message: ' + e.message); // ok
    }
    e instanceof RpcException2 ? e.msg : e.prop; // e.prop error
    e instanceof RpcException2 ? console.log('when true: ' + e.msg) : console.log('when false: ' + e.prop); // e.prop error
    // e.msg1 / e.prop error
    e instanceof RpcException2 ? console.log('when true: ' + (e instanceof RpcException3 ? e.msg1 : e.code)) : console.log('when false: '  + (e instanceof RpcException3 ? e.msg : e.prop));
    e instanceof RpcException2 ? console.log('when true: ' + (e instanceof RpcException3 ? e.msg : e.code)) : console.log('when false: '  + (e instanceof RpcException3 ? e.msg : e.message));
    console.log(errorMsg);
    (e as RpcException2).msg; // ok
    console.log('errorMsg: ' + (e as RpcException2).msg); // ok
    const temp = e;
    temp.prop; // error
    console.log('errorMsg: ' + temp.prop);  // error
    const temp1 = e as RpcException2;
    temp1.msg; // ok
    temp1.message; // ok
    console.log('errorMsg: ' + temp1.message); // ok
    console.log('errorMsg: ' + temp1.prop);  // error
  }
}

// import / extends Error
function a4() {
  try {
    throw new AError('msg');
  } catch (e) {
    let errorCode = 1;
    let errorMsg = '';
    if (e instanceof AError) {
      errorMsg = e.customProp;
      errorCode = e.code;
      console.log('a4: ' + e.prop); // error
    } else if (e instanceof AError1) {
      errorMsg = e.customProp;
      errorCode = e.code;
    } else {
      console.log('else prop: ' + e.prop); // error
      console.log('else message: ' + e.message); // ok
    }
    e instanceof AError ? e.customProp : e.prop; // e.prop error
    e instanceof AError ? console.log('when true: ' + e.customProp) : console.log('when false: ' + e.prop); // e.prop error
    // e.customProp1 / e.prop error
    e instanceof AError ? console.log('when true: ' + (e instanceof AError1 ? e.customProp1 : e.code)) : console.log('when false: '  + (e instanceof AError1 ? e.customProp : e.prop));
    e instanceof AError ? console.log('when true: ' + (e instanceof AError1 ? e.customProp : e.code)) : console.log('when false: '  + (e instanceof AError1 ? e.customProp : e.message));
    console.log(errorMsg);
    (e as AError).customProp; // ok
    console.log('errorMsg: ' + (e as AError).customProp); // ok
    const temp = e;
    temp.prop; // error
    console.log('errorMsg: ' + temp.prop);  // error
    const temp1 = e as AError;
    temp1.customProp; // ok
    temp1.message; // ok
    console.log('errorMsg: ' + temp1.message); // ok
    console.log('errorMsg: ' + temp1.prop);  // error
  }
}