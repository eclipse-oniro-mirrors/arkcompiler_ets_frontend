'use static'
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

export declare const wl_const: WorkerLocal<number>;
export declare const wl_nested: WorkerLocal<WorkerLocal<string>>;
export declare const ul_const: UserLocal<number>;
export declare const ns_const: myns.WorkerLocal<number>;
export declare const ns_eaw_const: myns.EAWorker;
export declare const cf_const: concurrency.Foo;
export declare const eaw_const: EAWorker;
export declare const arr_const: Array<number>;
export declare const map_const: Map<string, number>;

export function getWorkerLocal(wl: WorkerLocal<number>): WorkerLocal<number> {
    return wl;
}

export function getUserLocal(ul: UserLocal<string>): UserLocal<string> {
    return ul;
}

export function getConcurrentFoo(f: concurrency.Foo): concurrency.Foo {
    return f;
}

export function getEAWorker(w: EAWorker): EAWorker {
    return w;
}

export class WorkerLocalHolder {
    public value: WorkerLocal<number>;

    constructor(value: WorkerLocal<number>) {
        this.value = value;
    }

    public getValue(): WorkerLocal<number> {
        return this.value;
    }

    public setValue(value: WorkerLocal<number>): void {
    }
}

export class EAWorkerHolder {
    public worker: EAWorker;

    constructor(worker: EAWorker) {
        this.worker = worker;
    }
}

export class UserLocal<T> {
    public value: T;

    constructor(value: T) {
        this.value = value;
    }
}

export class UserLocalHolder {
    public ul: UserLocal<number>;

    constructor(ul: UserLocal<number>) {
        this.ul = ul;
    }
}

export interface WorkerLocalUser {
    wl: WorkerLocal<number>;
    use(wl: WorkerLocal<string>): WorkerLocal<number>;
}

export namespace myns {
    export class WorkerLocal<T> {
        public value: T;

        constructor(value: T) {
            this.value = value;
        }
    }

    export class EAWorker {
        public id: number;

        constructor(id: number) {
            this.id = id;
        }
    }
}

export namespace concurrency {
    export class Foo {
        public value: number;

        constructor(value: number) {
            this.value = value;
        }
    }
}

export const weakMap1:WeakMap<number,string>  = new WeakMap<number, string>();

export let weakMap2:WeakMap<number,string>  = new WeakMap<number, string>();

export const weakSet3:WeakSet<number> = new WeakSet<number>();

export let weakSet4:WeakSet<number> = new WeakSet<number>();
