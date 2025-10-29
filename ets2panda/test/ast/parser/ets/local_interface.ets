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

let myAssertEqual = (actualValue: boolean, expectValue: boolean) => {
  /* @@ label1 */interface LocalI {
    pass: boolean
    message: string
  }

  /* @@ label2 */class LocalC {
    pass: boolean = true
    message: string = "123"
  }
}

function test() {
  /* @@ label3 */interface LocalI {
    pass: boolean
    message: string
  }
  /* @@ label4 */class LocalC {
    pass: boolean = true
    message: string = "123"
  }

  if (true) {
    /* @@ label5 */interface LocalI {
      pass: boolean
      message: string
    }
    /* @@ label6 */class LocalC {
      pass: boolean = true
      message: string = "123"
    }
  }
}

class C {
  /* @@ label7 */interface LocalI {
    pass: boolean
    message: string
  }
  /* @@ label8 */class LocalC {
    pass: boolean = true
    message: string = "123"
  }

  foo(){
    /* @@ label9 */interface LocalI {
      pass: boolean
      message: string
    }
    /* @@ label10 */class LocalC {
      pass: boolean = true
      message: string = "123"
    }
  }
}

/* @@@ label1 Error Syntax error ESY0040: Illegal start of INTERFACE expression. */
/* @@@ label2 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@@ label3 Error Syntax error ESY0040: Illegal start of INTERFACE expression. */
/* @@@ label4 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@@ label5 Error Syntax error ESY0040: Illegal start of INTERFACE expression. */
/* @@@ label6 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@@ label7 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@@ label8 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@@ label9 Error Syntax error ESY0040: Illegal start of INTERFACE expression. */
/* @@@ label10 Error Syntax error ESY0040: Illegal start of CLASS expression. */