/**
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

class SameName {
    static /* @@ StaticFieldSameTy */a: int = 42;
    /* @@ NonStaticFieldSameTy */a: int = 32;
}

/* @@@ NonStaticFieldSameTy Node {
    "type": "ClassProperty",
    "key": {
      "type": "Identifier",
      "name": "a",
      "loc": {
        "start": {
          "line": 18,
          "column": 34,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 18,
          "column": 35,
          "program": "same_name_fields.ets"
        }
      }
    },
    "value": {
      "type": "NumberLiteral",
      "value": 32,
      "loc": {
        "start": {
          "line": 18,
          "column": 43,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 18,
          "column": 45,
          "program": "same_name_fields.ets"
        }
      }
    },
    "accessibility": "public",
    "static": false,
    "readonly": false,
    "declare": false,
    "optional": false,
    "computed": false
} */

/* @@@ StaticFieldSameTy Node {
    "type": "ClassProperty",
    "key": {
      "type": "Identifier",
      "name": "a",
      "loc": {
        "start": {
          "line": 17,
          "column": 38,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 17,
          "column": 39,
          "program": "same_name_fields.ets"
        }
      }
    },
    "value": {
      "type": "NumberLiteral",
      "value": 42,
      "loc": {
        "start": {
          "line": 17,
          "column": 47,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 17,
          "column": 49,
          "program": "same_name_fields.ets"
        }
      }
    },
    "accessibility": "public",
    "static": true,
    "readonly": false,
    "declare": false,
    "optional": false,
    "computed": false
} */

class DiffTypes {
    /* @@ DiffTyInt */a: int = 7;
    static /* @@ DiffTyStr */a: string = "a";
}

/* @@@ DiffTyInt Node {
    "type": "ClassProperty",
    "key": {
      "type": "Identifier",
      "name": "a",
      "loc": {
        "start": {
          "line": 106,
          "column": 23,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 106,
          "column": 24,
          "program": "same_name_fields.ets"
        }
      }
    },
    "value": {
      "type": "NumberLiteral",
      "value": 7,
      "loc": {
        "start": {
          "line": 106,
          "column": 32,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 106,
          "column": 33,
          "program": "same_name_fields.ets"
        }
      }
    },
    "accessibility": "public",
    "static": false,
    "readonly": false,
    "declare": false,
    "optional": false,
    "computed": false
} */

/* @@@ DiffTyStr Node {
    "type": "ClassProperty",
    "key": {
      "type": "Identifier",
      "name": "a",
      "loc": {
        "start": {
          "line": 107,
          "column": 30,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 107,
          "column": 31,
          "program": "same_name_fields.ets"
        }
      }
    },
    "value": {
      "type": "StringLiteral",
      "value": "a",
      "loc": {
        "start": {
          "line": 107,
          "column": 42,
          "program": "same_name_fields.ets"
        },
        "end": {
          "line": 107,
          "column": 45,
          "program": "same_name_fields.ets"
        }
      }
    },
    "accessibility": "public",
    "static": true,
    "readonly": false,
    "declare": false,
    "optional": false,
    "computed": false
} */

function main(): void { 
    arktest.assertEQ(SameName.a, 42);
    arktest.assertEQ((new SameName()).a, 32);
    arktest.assertEQ(DiffTypes.a, "a");
    arktest.assertEQ((new DiffTypes()).a, 6);
}
