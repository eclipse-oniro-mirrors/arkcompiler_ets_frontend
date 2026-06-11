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

import { I, A, B } from './fixtures/BaseClass';

interface BaseI {}

class ParentC implements BaseI {
  par1?: string = 'parentC';
}

class SonC extends ParentC {}

let objP: ParentC = new ParentC();
let sameObj: ParentC = objP as ParentC;

let childAsInterface: BaseI = new SonC();
if (childAsInterface instanceof SonC) {
  let guarded: SonC = childAsInterface as SonC;
}

interface FlowBaseI {}

class FlowParent implements FlowBaseI {
  name: string = 'parent';
}

class FlowSon extends FlowParent {}

let safeUpcastBase: FlowBaseI = new FlowSon();
let safeUpcast: FlowParent = safeUpcastBase as FlowParent;
let sameType: FlowParent = new FlowParent() as FlowParent;

function numericPositiveCast(value: number | string): number {
  let noWarnPlain: number = value as number;
  return noWarnPlain;
}

function positiveCase() {
  let a: number = 123;
  a++;
  let b: number = a as number;
}

interface GuardBaseI {}

class GuardParent implements GuardBaseI {
  name: string = 'parent';
}

class GuardParent1 implements GuardBaseI {
  name: string = 'parent1';
}

function positiveGuard(value: GuardBaseI): void {
  if (value instanceof GuardParent1) {
    let guarded: GuardParent1 = value as GuardParent1;
  }
}

function base(): void {
    let a: A = new A();
    let i: I = a;
    if (i instanceof B) {
        let b: B = i as B;
    }
}

function inIfIfBranch(): void {
    let a: A = new A();
    let i: I = a;
    if (i instanceof B) {
        if (a !== undefined) {
            let b: B = i as B;
        }
    }
}

function inElseElseBranch(): void {
    let a: A = new A();
    let i: I = a;
    if (!(i instanceof B)) {
        let b = i;
    } else {
        if (a === undefined) {
            return;
        } else {
            let b: B = i as B;
        }
    }
}

function inIfElseBranch(): void {
    let a: A = new A();
    let i: I = a;
    if (i instanceof B) {
        if (a === undefined) {
            return;
        } else {
            let b: B = i as B;
        }
    }
}

function bothElseIfBranch(): void {
    let a: A = new A();
    let i: I = a;
    if (!(i instanceof B)) {
        let b = i;
    } else {
        if (a !== undefined) {
            let b: B = i as B;
        }
    }
}

const cond1 = true;
const cond2 = false;

function binaryAnd(): void {
    let a: A = new A();
    let i: I = a;;
    if (i instanceof A && cond1) {
        let b: A = i as A;
    }
}

function binaryAndAnd(): void {
    let a: A = new A();
    let i: I = a;;
    if (i instanceof A && cond1 && cond2) {
        let b: A = i as A;
    }
}

interface RelabelSafeBaseI {}
class RelabelSafeParentC implements RelabelSafeBaseI {}
class RelabelSafeSonC extends RelabelSafeParentC {}

function takeRelabelSafeParentC(value: RelabelSafeParentC): void {}

let relabelSafeParentSource: RelabelSafeBaseI = new RelabelSafeParentC();
takeRelabelSafeParentC(relabelSafeParentSource as RelabelSafeParentC);

let relabelSafeSonSource: RelabelSafeBaseI = new RelabelSafeSonC();
let relabelSafeUpcast: RelabelSafeParentC = relabelSafeSonSource as RelabelSafeParentC;

let relabelSafeMap: Map<string, RelabelSafeParentC> = new Map<string, RelabelSafeParentC>();
relabelSafeMap.set('key', new RelabelSafeParentC());
let relabelSafeMapValue: RelabelSafeParentC = relabelSafeMap.get('key') as RelabelSafeParentC;

let relabelSafeSet: Set<RelabelSafeParentC> = new Set<RelabelSafeParentC>();
relabelSafeSet.add(new RelabelSafeParentC());
relabelSafeSet.forEach((value: RelabelSafeParentC) => {
  let relabelSafeSetValue: RelabelSafeParentC = value as RelabelSafeParentC;
});

let relabelSafeArray: Array<RelabelSafeParentC> = new Array<RelabelSafeParentC>();
relabelSafeArray.push(new RelabelSafeParentC());
let relabelSafeArrayValue: RelabelSafeParentC = relabelSafeArray.pop() as RelabelSafeParentC;
