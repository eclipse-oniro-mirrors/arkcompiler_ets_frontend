/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class Nodes<K extends Lista<Object>, V> {
    left: Nodes<K, V>;
    key: K;
    value: V;
    height: int;
    right: Nodes<K, V>;
    constructor(
        left: Nodes<K, V>,
        key: K,
        value: V,
        height: int,
        right: Nodes<K, V>
        )
    {
        this.key = key;
        this.value = value;
        this.left = left;
        this.right = right;
        this.height = height;
    }
}

interface Lista <T>  {
    pushFront(e: T): void;
    popFront(): T;
    pushBack(e: T): void;
    popBack(): T;
    size(): int;
    at(index: int): T;
    has(e: T): boolean;
    forEach(fn: (e: T) => T): Lista<T>;
    map<U, LU extends Lista<U>>(fn: (e: T) => U): LU;
    fold(combine: (lhs: T, rhs: T) => T): T;
    foldWith<U>(combine: (lhs: U, rhs: T)=> U, initVal: U): U;
    filter(filterCond: (e: T)=> boolean): Lista<T>;
    sort(comparator: (lhs: T, rhs: T)=> boolean): Lista<T>;
}

class A<T>{
    j: A<T>;
    constructor(j: A<T>) {
        this.j = j;
    }
}

class B<K extends Lista<Object>, V>{
    j: Nodes<K, V>;
    i: Nodes<K, V>;
    constructor(
        j: Nodes<K, V>,
        i: Nodes<K, V>
        )
    {
        this.j = j;
        this.i = i;
    }
    foo(): void{
        this.j.right.left.right.key.popFront();
        this.j = this.i;
     }
}
