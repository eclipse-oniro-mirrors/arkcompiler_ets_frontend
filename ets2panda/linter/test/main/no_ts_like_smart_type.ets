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

class A {
    private static instance:A = new A();
        static get():A {
        return A.instance; //
    }
}

class AA {
  public static instance?: number;

  getInstance(): number {
    if (!AA.instance) {
      return 0;
    }
    return AA.instance; // Error
  }
}

class AA1 {
  public static instance : Number | String | Object = "smart cast";
  getInstance(): Number {
    if (!(AA1.instance instanceof Number)) {
      return 0;
    }
    return AA1.instance; // Error
  }
}

class AA2 {
  public instance : Number | String | Object= 'smart cast';
  getInstance(): Number {
    if (!(this.instance instanceof Number)) {
      return 0;
    }
    return this.instance; // Error
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
    return this.instance; // Error
  }
}

function foo2(str:string){
}
function foo22(str:number|string){
}
function foo(spec :string|number){
  foo2(spec) // Error
}
function foo3(spec :string|number){
 if (typeof spec == "string") {
    foo2(spec) // Error
 }
}
function foo3(spec :string|number){
 if (typeof spec == "string") {
    foo22(spec)
 }
}

const input = true;
if (typeof input === 'boolean') {
} else {
  let result = input as string | number | null;  // Error
}

async function taskInfo():Promise<void> {//nopass
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
       priority = threadInfo.priority; // Error
      }
      hilog.info(0x0000, 'testTag-----',"getTaskPoolInfo: --tid is:" + tid + ", taskIds is:" + taskIds + ", priority is:" + priority);
    }
    let taskIS: Array<taskpool.TaskInfo> = Array.from<taskpool.TaskInfo>(taskpoolInfo.taskInfos)
    for (let taskInfo of taskIS) {
      taskId = taskInfo.taskId;
      state = taskInfo.state;
      if (taskInfo.duration != undefined) {
        duration = taskInfo.duration as number;  // Error
        name = taskInfo.name;
      }
      hilog.info(0x0000, 'testTag-----',"getTaskPoolInfo: --taskId is:" + taskId + ", state is:" + state + ", duration is:" + duration +
        ", name is:" + name);
    }
  }
}

class A {
    f(): number| null {
        return 1
    }
}

class B {
    a?:A = new A()
}
let b = new B()
if(b.a && b.a.f() !== null){ //Error
    console.log('23')
}

if (b.a.f() !== null) { //Error
  console.log('warn');
}
if (b.a?.f() !== null) {
  console.log('ok');
}
function check(obj?: A) {
  if (obj.f() !== null) { //Error
    console.log('warn');
  }
}
if ((b.a?.f() ?? 0) > 0) {
  console.log('ok');
}
const val = b.a.f(); //Error


function foo4(uri: string | number | boolean) : string {
  return uri as string
}

class AA {
  public static starUrl(url: string)  {
    if (url?startsWith('http')) {
      url = `https:/test?appid=2000?url=${foo4(url)}`;
    }
  }
}


function sleep<T = void>(ms: number): PromiseLike {
  return new Promise(
    (resolve: (value: T | PromiseLike) => void): number => setTimeout(resolve, ms)
  );
}

type TypeA = (key: string) => string | undefined

class ClassA {
    static sA?: TypeA = undefined

    static funA(key: string): string | undefined {
        if (ClassA.sA) {
            return ClassA.sA(key) //error
        }
        else {
            return undefined;
        }
    }
}

class A {
    b?: boolean

    isRelease(): boolean {
        if (this.b != undefined) {
            return this.b //error
        }
    }
}

export class K {
    _t:string
    constructor(t:string) {
        this._t = t;
    }
}

export class P extends K {
    constructor(t:string){
        super(t)
    }
    toString(): string {
        return this._t // legal
    }
}

export class H {
  protected _t:string
  constructor(t:string) {
      this._t = t;
  }
}

export class T extends H {
    constructor(t:string) {
      super(t)
    }
}

export class N extends T {
    constructor(t:string){
        super(t)
    }
    toString(): string {
        return this._t // legal
    }
}

type AsyncCB<T> = () => T | Promise<T>;

class AsyncLock {
    lockAsync<T>(callback: AsyncCB<T>): Promise<T> {
    }
}

export class AB {
    private count_: number = 0
    public lock_: AsyncLock = new AsyncLock();

    public async getCount(): Promise<number> {
        return this.lock_.lockAsync(() => {
            return this.count_; //legal
        })
    }
} 