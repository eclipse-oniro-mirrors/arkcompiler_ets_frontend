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

// global variables
let a: number | undefined = 1;
if (a != undefined) {
	a++; // Error: arkts-no-ts-like-smart-type
}
a++; // Error: arkts-no-ts-like-smart-type

class AW {
  private static instance:AW = new AW();
  static get():AW {
    return AW.instance;
  }
}

class AA {
  public static instance?: number;
  public static instance2: number | String = 1;
  public instance3?: number;
  public instance4: number | String = 1;

  getInstance(): number {
    if (!AA.instance) {
      return 0;
    }
    return AA.instance; // Error: arkts-no-ts-like-smart-type
  }

  getInstance2(): number {
    if (AA.instance2 instanceof String) {
      return 0;
    }
    return AA.instance2; // Error: arkts-no-ts-like-smart-type
  }

  getInstance3(): number {
    if (!this.instance3) {
      return 0;
    }
    return this.instance3; // Error: arkts-no-ts-like-smart-type
  }

  getInstance4(): number {
    if (this.instance4 instanceof String) {
      return 0;
    }
    return this.instance4; // Error: arkts-no-ts-like-smart-type
  }
}

class AA1 {
  public static instance : Number | String | Object = "smart cast";
  getInstance(): Number {
    if (!(AA1.instance instanceof Number)) {
      return 0;
    }
    return AA1.instance; // Error: arkts-no-ts-like-smart-type
  }
}

class AA2 {
  public instance : Number | String | Object= 'smart cast';
  getInstance(): Number {
    if (!(this.instance instanceof Number)) {
      return 0;
    }
    return this.instance; // Error: arkts-no-ts-like-smart-type
  }
}

class AA3 {
  public instance : number | String | Object = 'string';
  getInstance(): number {
    if (this.instance instanceof String) {
      return 0;
    } else if (this.instance instanceof Object) {
      return 1;
    }
    return this.instance; // Error: arkts-no-ts-like-smart-type
  }
}

function foo2(str:string){}
function foo22(str:number|string){}
function foo(spec :string|number){
    foo2(spec)
}
function foo3(spec :string|number){
    if (typeof spec == "string") {
        foo2(spec) // Error: arkts-no-ts-like-smart-type
    }
}

const input = true;
if (typeof input === 'boolean') {
} else {
  let result = input as string | number | null;  // Error: arkts-no-ts-like-smart-type
}

async function taskInfo():Promise<void> {
  for (let i:int = 0; i < 1000; i++) {
    let taskpoolInfo: taskpool.TaskPoolInfo = taskpool.getTaskPoolInfo();
    hilog.info(0x0000, 'testTag-----',`getTaskPoolInfo: , ${JSON.stringify(taskpoolInfo)}`)
    let tid: number = 0;
    let taskIds: number[] = [];
    let priority: int = 0;
    let taskId: number = 0;
    let state: number = 0;
    let duration: number = 0;
    let name: string = "";
    let threadIS: Array<taskpool.ThreadInfo> = Array.from<taskpool.ThreadInfo>(taskpoolInfo.threadInfos)
    for (let threadInfo of threadIS) {
      tid = threadInfo.tid;
      if (threadInfo.taskIds != undefined && threadInfo.priority != undefined) {
       priority = threadInfo.priority; // Error: arkts-no-ts-like-smart-type
      }
      hilog.info(0x0000, 'testTag-----',"getTaskPoolInfo: --tid is:" + tid + ", taskIds is:" + taskIds + ", priority is:" + priority);
    }
    let taskIS: Array<taskpool.TaskInfo> = Array.from<taskpool.TaskInfo>(taskpoolInfo.taskInfos)
    for (let taskInfo of taskIS) {
      taskId = taskInfo.taskId;
      state = taskInfo.state;
      if (taskInfo.duration != undefined) {
        duration = taskInfo.duration as number;  // Error: arkts-no-ts-like-smart-type
        name = taskInfo.name;
      }
      hilog.info(0x0000, 'testTag-----',"getTaskPoolInfo: --taskId is:" + taskId + ", state is:" + state + ", duration is:" + duration +
        ", name is:" + name);
    }
  }
}

class A {
    f(): number| null {
        return 1;
    }
}

class B {
    a?:A = new A();
}

let b = new B();

if(b.a && b.a.f() !== null){ // Error: arkts-no-ts-like-smart-type
    console.log('23')
}

if (b.a.f() !== null) { // Error: arkts-no-ts-like-smart-type
  console.log('warn');
}

const val = b.a.f(); // Error: arkts-no-ts-like-smart-type

function check(obj?: A) {
  if (obj.f() !== null) {
    console.log('warn');
  }
}

type TypeA = (key: string) => string | undefined

class ClassA {
    static sA?: TypeA = undefined

    static funA(key: string): string | undefined {
        if (ClassA.sA) {
            return ClassA.sA(key) // Error: arkts-no-ts-like-smart-type
        }
        else {
            return undefined;
        }
    }
}

class AT {
    b?: boolean

    isRelease(): boolean {
        if (this.b != undefined) {
            return this.b; // Error: arkts-no-ts-like-smart-type
        }
    }
}

class AG {
    h?: number;
    f(): number | null {
      return 1;
    }
}

function check2(obj?: AG) {
    obj.f();

    if (obj == null) return;
    obj.f(); // valid(guarded)
}

function check35(obj?: AG) {
    if (obj.h == 3) {
      console.log('3');
    }
}

function check43(obj?: AG) {
    const val = obj.h;
    return val;
}

let a: number | string = 42;
a++;  // Error: arkts-no-ts-like-smart-type, global variable

let a2: number | string = 42;
const b2 = a++;  // Error: arkts-no-ts-like-smart-type, global variable
const b3 = --a; // Error: arkts-no-ts-like-smart-type, global variable

class ay {
    a: number | string = 42;
}

function boo() {
    let aa: ay = new ay();
    aa.a++;   // Error: arkts-no-ts-like-smart-type, object prop
}

class A3 {
    a: number = 1.0;
}

class B3 {
    static bbb?: number = 1;
    b: A3 | null = new A3();
    test1(b: A3 | null): number | undefined {
        return undefined;
    }

    test2(b: A3): number | undefined {
        return undefined;
    }

    test3(a:number, b: A3): number | undefined {
        return undefined;
    }
}

function aoo() {
    let bObj = new B3();
    if (bObj.b != null) {
        bObj.b.a = 2.0;   // Error: arkts-no-ts-like-smart-type
    }
}

function coo() {
    let bObj = new B3();
    if (bObj.b instanceof A3) {
        bObj.b.a = 2.0;   // Error: arkts-no-ts-like-smart-type
    }
}

function foo3(bObj: B3 | undefined, ccc: number | undefined) {
    if (bObj != undefined && bObj.b != undefined) {
        bObj.b.a = 2.0; // Error: arkts-no-ts-like-smart-type
        bObj.b.a; // Error: arkts-no-ts-like-smart-type
        const a1 = bObj.b?.a; // Ok
        let a2: number = a1; //  Error: arkts-no-ts-like-smart-type
        a2 =  bObj.b?.a;	 //  Error: arkts-no-ts-like-smart-type
        const b1 = bObj.b; // Ok
        let b2: A3 = b1; //  Error: arkts-no-ts-like-smart-type
        b2 = b1; //  Error: arkts-no-ts-like-smart-type
        let b3: A3 | undefined = b1; //  Error: arkts-no-ts-like-smart-type
        b3 = b1; //  Error: arkts-no-ts-like-smart-type
        const b: A3 = bObj.b; // Error: arkts-no-ts-like-smart-type

        bObj.test2(bObj.b); // Error: arkts-no-ts-like-smart-type
        bObj.test3(1, bObj.b); // Error: arkts-no-ts-like-smart-type

        if (ccc != undefined) { // Ok
            const d: number = ccc; // Ok
        }

        if (B3.bbb != undefined) { // Ok
            const e: number = B3.bbb; // Error: arkts-no-ts-like-smart-type
        }
    }
}

function foo4(ccc: number | undefined) {
    if (B3.bbb) { // Ok
        return B3.bbb;
    }
    if (!B3.bbb) { // Ok
        return B3.bbb;
    }
    if (B3.bbb != undefined) { // Ok
        return B3.bbb;
    }
    if (B3.bbb instanceof number) { // Ok
        return B3.bbb;
    }
    if (typeof B3.bbb === 'number') { // Ok
        return B3.bbb;
    }
    if (typeof ccc === 'number') { // Ok
        return ccc;
    }
}

class A4 {
    public instance : Number | String | Object= 'smart cast'; // Ok
    public aaa:Number | undefined = 1;
    public aClass: A3 | undefined;
    getInstance(): Number {
        if (!(this.instance instanceof Number)) { // Ok
            return 0;
        }

        if (this.aaa != undefined) { // Ok
            return this.aaa; //  Error: arkts-no-ts-like-smart-type
        }

        if (this.aClass?.a) { // Ok
            this.test1(this.aClass?.a); // Ok
            this.test2(this.aClass?.a); // Ok
            this.test3(this.aClass?.a);
        }
        return this.instance; // Error: arkts-no-ts-like-smart-type
    }

    test1(a?: number) {}
    test2(a: number | undefined) {}
    test3(a: number) {}
}

class C {}

class Br {
    bb: number = 1;
    getBB() {
        return new C();
    }
}

class Ar {
    a: Br | undefined;
    b: C | undefined;
    constructor() {
        this.a = new Br();
        this.b = this.a.getBB(); // Error: arkts-no-ts-like-smart-type
    }
}

class ef {
    a: number = 1.0;
}

function roo(aObj: ef | null) {
    if (typeof aObj === 'ef') {
        aObj.a = 2.0;
    }
}

class Ay {
    private connectOptions?: Promise<void>;

    private connect(): Promise<void> {
        this.connectOptions = new Promise(() => { });
        return this.connectOptions; // Error: arkts-no-ts-like-smart-type
    }
}

class SmartTypeAliasBase {
    num: Number | undefined = 0;
}

class SmartTypeAliasSon extends SmartTypeAliasBase {}

type SmartTypeAliasBaseAlias = SmartTypeAliasBase;
type SmartTypeAliasMaybeBase = SmartTypeAliasBase | undefined;
type SmartTypeAliasBaseOrSon = SmartTypeAliasBase | SmartTypeAliasSon;

function smartTypeAliasFromParameter(aliasParam: SmartTypeAliasMaybeBase): Number {
    if (aliasParam instanceof SmartTypeAliasBase) {
        return aliasParam.num as Number; // Ok
    }
    return 0;
}

function smartTypeAliasFromLocal(): Number {
    let aliasLocal: SmartTypeAliasBaseAlias | undefined = new SmartTypeAliasBase();
    if (aliasLocal.num instanceof Number) {
        return aliasLocal.num; // Error: arkts-no-ts-like-smart-type
    }
    return 0;
}

function smartTypeAliasFromNonNullableUnion(): Number {
    let aliasLocal: SmartTypeAliasBaseOrSon = new SmartTypeAliasSon();
    if (aliasLocal.num instanceof Number) {
        return aliasLocal.num; // Error: arkts-no-ts-like-smart-type
    }
    return 0;
}

function smartTypeAliasFromNestedParameter(aliasParam: SmartTypeAliasMaybeBase): Number {
    if (aliasParam instanceof SmartTypeAliasBase) {
        if (aliasParam.num !== undefined) {
            return aliasParam.num as Number; // Ok
        }
    }
    return 0;
}

function smartTypeAliasFromNestedLocal(): Number {
    let aliasLocal: SmartTypeAliasBaseAlias | undefined = new SmartTypeAliasBase();
    if (aliasLocal.num instanceof Number) {
        if (aliasLocal.num !== undefined) {
            return aliasLocal.num; // Error: arkts-no-ts-like-smart-type
        }
    }
    return 0;
}

class SmartTypePr9977Main {
    num: Number | undefined = 0;
    public pubNum: Number | undefined = undefined;
    static staNum: Number | undefined = undefined;
    nested?: SmartTypePr9977Nested;

    test(): Number {
        if (this.num) {
            return this.num; // Error: arkts-no-ts-like-smart-type
        }
        if (SmartTypePr9977Main.staNum) {
            return SmartTypePr9977Main.staNum; // Error: arkts-no-ts-like-smart-type
        }
        return 0;
    }
}

class SmartTypePr9977Nested {
    num: Number | undefined = 0;
    public deepNested?: SmartTypePr9977Deep;
    static deepNestedStatic?: SmartTypePr9977Deep;
}

class SmartTypePr9977Deep {
    num: Number | undefined = 0;
}

function smartTypePr9977ReturnFromMain(a: SmartTypePr9977Main | undefined): Number {
    if (a instanceof SmartTypePr9977Main) {
        if (a.num instanceof Number) {
            return a.num; // Error: arkts-no-ts-like-smart-type
        } else if (a.pubNum instanceof Number) {
            return a.pubNum; // Error: arkts-no-ts-like-smart-type
        } else if (SmartTypePr9977Main.staNum instanceof Number) {
            return SmartTypePr9977Main.staNum; // Error: arkts-no-ts-like-smart-type
        }
    }
    return 0;
}

function smartTypePr9977StaticPropertyAs(a: SmartTypePr9977Main | undefined): Number {
    if (a instanceof SmartTypePr9977Main) {
        if (SmartTypePr9977Main.staNum instanceof Number) {
            return SmartTypePr9977Main.staNum as Number; // Ok
        }
    }
    return 0;
}

function smartTypePr9977AssignmentFromInstanceProperty(a: SmartTypePr9977Main): void {
    if (a.num instanceof Number) {
        const narrowedNumber: Number = a.num; // Error: arkts-no-ts-like-smart-type
        const explicitUnion: Number | undefined = a.num; // Ok
    }
}

function smartTypePr9977OptionalParamNestedProperty(a?: SmartTypePr9977Main): Number {
    if (a !== undefined) {
        if (a.nested instanceof SmartTypePr9977Nested) {
            const nestedNumber: Number = a.nested.num as Number; // Error: arkts-no-ts-like-smart-type
            const optionalNumber = a.nested?.num; // Ok
            console.log(`${a.nested?.num}`); // Ok
            console.log(`${a.nested.num}`); // Error: arkts-no-ts-like-smart-type
            return a.nested.num as Number; // Error: arkts-no-ts-like-smart-type
        }
    }
    return 1;
}

function smartTypePr9977MixedAsAndInstanceof(nestedObj?: SmartTypePr9977Nested): Number {
    let maybeMain1: SmartTypePr9977Main | undefined = new SmartTypePr9977Main();
    if (maybeMain1.num instanceof Number) {
        return maybeMain1.num as Number; // Ok
    }

    if (nestedObj instanceof SmartTypePr9977Nested) {
        if (nestedObj.num instanceof Number) {
            return nestedObj.num as Number; // Ok
        }
        if (nestedObj.deepNested instanceof SmartTypePr9977Deep) {
            return nestedObj.deepNested.num as Number; // Error: arkts-no-ts-like-smart-type
        }
        if (SmartTypePr9977Nested.deepNestedStatic instanceof SmartTypePr9977Deep) {
            return SmartTypePr9977Nested.deepNestedStatic.num as Number; // Error: arkts-no-ts-like-smart-type
        }
    }
    return 1;
}

function smartTypePr9977UnionBranchReturn(): Number {
    let maybeMain1: SmartTypePr9977Main | undefined = new SmartTypePr9977Main();
    if (maybeMain1.num instanceof Number) {
        return maybeMain1.num; // Error: arkts-no-ts-like-smart-type
    }

    let maybeMain2: SmartTypePr9977Main | undefined = new SmartTypePr9977Main();
    if (maybeMain2.num instanceof Number) {
        return maybeMain2.num as Number; // Ok
    }
    return 0;
}

class SmartTypeSampleUnionA {
    value: number = 1.0;
}

class SmartTypeSampleUnionB {
    value: number = 2.0;
}

class SmartTypeSampleUnionC {
    value: number = 3.0;
    extra: string = 'c';
}

class SmartTypeSampleMultiUnionHolder {
    node: SmartTypeSampleUnionA | SmartTypeSampleUnionB | SmartTypeSampleUnionC = new SmartTypeSampleUnionC();

    getNode(): SmartTypeSampleUnionC {
        if (this.node instanceof SmartTypeSampleUnionA) {
            return new SmartTypeSampleUnionC();
        } else if (this.node instanceof SmartTypeSampleUnionB) {
            return new SmartTypeSampleUnionC();
        }
        return this.node; // Error: arkts-no-ts-like-smart-type
    }
}

class SmartTypeSampleLeaf {
    num: Number | undefined = 0;
}

class SmartTypeSampleSubLeaf extends SmartTypeSampleLeaf {}

class SmartTypeSampleContainer {
    child: SmartTypeSampleLeaf | undefined = new SmartTypeSampleLeaf();
    public childOrSub: SmartTypeSampleLeaf | SmartTypeSampleSubLeaf = new SmartTypeSampleLeaf();
}

class SmartTypeSampleRoot {
    container: SmartTypeSampleContainer | undefined = new SmartTypeSampleContainer();
}

type SmartTypeSampleRootAlias = SmartTypeSampleRoot;

function smartTypeSampleNestedMember(root: SmartTypeSampleRoot | undefined): Number {
    if (root instanceof SmartTypeSampleRoot) {
        if (root.container instanceof SmartTypeSampleContainer) {
            if (root.container.child instanceof SmartTypeSampleLeaf) { // Error: arkts-no-ts-like-smart-type
                return root.container.child.num; // Error: arkts-no-ts-like-smart-type
            }
        }
    }
    return 0;
}

function smartTypeSampleNestedAlias(): Number {
    let localRoot: SmartTypeSampleRootAlias | undefined = new SmartTypeSampleRoot();
    if (localRoot.container instanceof SmartTypeSampleContainer) {
        if (localRoot.container.child instanceof SmartTypeSampleLeaf) { // Error: arkts-no-ts-like-smart-type
            return localRoot.container.child.num; // Error: arkts-no-ts-like-smart-type
        }
    }
    return 0;
}

function smartTypeSampleNullableOwnerAs(holder: SmartTypeSampleContainer): Number {
    if (holder.child instanceof SmartTypeSampleLeaf) {
        return holder.child.num as Number; // Error: arkts-no-ts-like-smart-type
    }
    return 0;
}
