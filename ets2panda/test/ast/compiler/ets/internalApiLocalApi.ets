/**
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

import { annotation } from 'arkruntime'

@annotation.AccessRestriction(["allowed.test"])
@Retention("SOURCE")
export @interface TestRestriction {}

@annotation.AccessRestriction(["allowed"])
export @interface myapi {}

@annotation.AccessRestriction(["allowed.custom"])
@Retention("SOURCE")
export @interface TeamOnly {}

@TestRestriction
export namespace restrictedNs {
    @TestRestriction
    export function restrictedFunction(): void {}

    @TestRestriction
    export let restrictedValue: int = 0
}

export class RestrictedMembers {
    @TestRestriction
    public value: int = 1

    public constructor() {}

    @TestRestriction
    public ping(): void {}
}

export class ConstructorRestrictedApi {
    @myapi
    public constructor() {}
}

@TeamOnly
export namespace customNs {
    @TeamOnly
    export function customFunction(): void {}

    @TeamOnly
    export let customValue: int = 0
}

export class RestrictedCtor {
    @TeamOnly
    public constructor() {}
}

export class RestrictedClass {
    @TeamOnly
    public field: int = 0

    @TeamOnly
    public method(): void {}
}

@TeamOnly
export interface RestrictedIface {}

@TeamOnly
export let restrictedGlobal: int = 0
