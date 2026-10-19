'use static';
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"),
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

class Class {

    getName(): string;

    getSuper(): Class | undefined;

    isSubtypeOf(other: Class): boolean;

    getLinker(): RuntimeLinker;

    getFixedArrayComponentType(): Class | undefined;

    getInterfaces(): FixedArray<Class>;

    initialize(): void;

    static ofAny(obj: Any): Class | undefined;

    static of(obj: Object | null): Class;

    static from<T>(): Class;

    static current(): Class;

    static ofCaller(): Class | undefined;

    createInstance(): Object;

    getDescriptor(): string;

    isEnum(): boolean;

    isInterface(): boolean;

    isFixedArray(): boolean;

    isUnion(): boolean;

    getInstanceMethods(): FixedArray<reflect.InstanceMethod>;

    getInstanceMethod(name: string, signature?: FixedArray<Class>): reflect.InstanceMethod | undefined;

    getStaticMethods(): FixedArray<reflect.StaticMethod>;

    getStaticMethod(name: string, signature?: FixedArray<Class>): reflect.StaticMethod | undefined;

    getInstanceFields(): FixedArray<reflect.InstanceField>;

    getInstanceField(name: string): reflect.InstanceField | undefined;

    getStaticFields(): FixedArray<reflect.StaticField>;

    getStaticField(name: string): reflect.StaticField | undefined;

    getConstructors(): FixedArray<reflect.Constructor>;

    getUnionConstituentTypes(): FixedArray<Class> | undefined;

    isFinal(): boolean;

    isAbstract(): boolean;

    isPrimitive(): boolean;
}
