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

class A {
    fld: string = "hello"

    constructor(p: string) {
        this.fld = p
    }

    constructor() {
    }

    meth(): string {
        return "hello"
    }
}

class B extends A {
    constructor(p: string) {
        super(p)
    }

    constructor() {
        let v = super.meth()
        this(v)
    }
}

class C extends A {
    constructor(p: () => string) {
        super(p())
    }

    constructor() {
        let v = super.meth
        this(v)
    }
}

class D extends A {
    constructor() {
        let v = super.meth()
        super()
    }
}

class E extends A {
    constructor(p: string) {
        super(p)
    }

    constructor() {
        let v = super.fld
        this(v)
    }
}

/* @@? 37:17 Error Semantic error ESE0679: 'super' cannot be referenced before super() call or this() call. */
/* @@? 48:17 Error Semantic error ESE0679: 'super' cannot be referenced before super() call or this() call. */
/* @@? 55:17 Error Semantic error ESE0679: 'super' cannot be referenced before super() call or this() call. */
/* @@? 66:23 Error Semantic error ESE0377: Class field 'fld' defined by the parent class is not accessible in the child class via super. */
