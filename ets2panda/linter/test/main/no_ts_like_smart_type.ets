/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
	a++; // Error
}
a++; // Error

class AW {
    private static instance:AW = new AW();
        static get():AW {
        return AW.instance; //
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
function foo6(spec :string|number){
 if (typeof spec == "string") {
    foo22(spec) // Ok
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
if(b.a && b.a.f() !== null){ // Error
    console.log('23')
}

if (b.a.f() !== null) { // Error
  console.log('warn');
}
if (b.a?.f() !== null) {
  console.log('ok');
}
function check(obj?: A) {
  if (obj.f() !== null) { // Error
    console.log('warn');
  }
}
if ((b.a?.f() ?? 0) > 0) {
  console.log('ok');
}
const val = b.a.f(); // Error


function foo4(uri: string | number | boolean) : string {
  return uri as string
}

class AA5 {
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
            return ClassA.sA(key) // Error
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
            return this.b; // Error
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

class AG {
h?: number;
f(): number | null {
  return 1;
}
}

function check5(obj?: AG) {
if (obj) {
  console.log(obj.f()); // valid(guarded)
}
if (obj !== undefined) {
  obj.f(); // valid(guarded)
}
}

function check4(obj?: AG) {

if (!obj) {
  return;
}
obj.f(); // valid(guarded)
}
function check2(obj?: AG) {
obj.f(); // Error

if (obj == null) return;
obj.f(); // valid(guarded)

}
function check3(obj?: AG) {

return obj ? obj.f() : null; // guarded in ternary

}

function check35(obj?: AG) {

if (obj.h == 3) { // Error
  console.log('3');
}

}

function check36(obj?: AG) {

if (obj && obj.h == 3) { // valid(guarded)
  console.log('3');
}

}

function check43(obj?: AG) {
const val = obj.h; // NOT guarded
return val;
}

}

let NextID: number = 1;

class downloadFilesData {
  id: number;
  url: string;
  fileStatus: number;
  downloadTime: number;

  constructor(
    url: string = '',
    fileStatus: number = 0,
    downloadTime: number = 0,
  ) {
    this.id = NextID++;
    this.url = url;
    this.fileStatus = fileStatus;
    this.downloadTime = downloadTime;
  }
}

class Downloader {
  downloadFileArray: downloadFilesData[] = [];

  loadInitializationDataSource() {
    let stringData = JSON.stringify([
      {
        "url": "https://example.com/file1.zip",
        "fileStatus": 1,
        "downloadTime": 1734931200
      },
      {
        "url": "https://example.com/file2.mp4",
        "fileStatus": 0,
        "downloadTime": 0
      },
      {
        "url": "https://example.com/file3.pdf",
        "fileStatus": 2,
        "downloadTime": 1734931300
      }
    ]);

    let data: [] = JSON.parse(stringData) as [];

    for (let i = 0; i < data.length; i++) {
      const downloadItemData = data[i] as downloadFilesData;
      let downloadData: downloadFilesData = new downloadFilesData(
        downloadItemData.url,
        downloadItemData.fileStatus,
        downloadItemData.downloadTime
      );
      this.downloadFileArray.push(downloadData);
    }
  }
}

function isOperator(operator) {
  return (operator === '+' || operator === "-")
}
function test(value: string) {
  if (isOperator(value)) { //no error
    console.log('5')
  }
}

class Temp {
  public ret: boolean = true;

  async checkAppItem(elementNameVal: Want) {
    logger.info(TAG, 'checkAppItem in bundleName:' + elementNameVal.bundleName + ' | abilityName:' +
    elementNameVal.abilityName);
    let want: Want = {
      'bundleName': elementNameVal.bundleName,
      'abilityName': elementNameVal.abilityName
    };
    let userId: UserId = { localId: 0 };
    let retVal = await accountManager.getAccountUserId(userId);
    if (!retVal) {
      logger.warn(TAG, 'checkAppItem getAccountUserId fail!');
      this.ret = false;
      return this.ret;
    }
  }
}

// 1 - union return type
declare type ResourceColor = number | string;

class buttonBgTestParent {
  public buttonBgColor: ResourceColor = '#FF000000'
}

class buttonBgTest extends buttonBgTestParent {
  getBgColor(test: string): ResourceColor | undefined {
    if (test) {
      return this.buttonBgColor; //legal
    }
    return undefined;
  }
}

// 2 - clearTimeout (stdlib func)

class Common {
  static isInvalid(obj: unknown): boolean {
    return obj == null;
  }

  private checkRunnable(run: number | undefined): number | undefined {
    if (Common.isInvalid(run)) {
      return undefined
    }
    clearTimeout(run); // legal
    return undefined
  }

}

// 3 - method decl return

class CardUtil {
  public static isValid(card: string): boolean {
    return card?.length >= 6
  }

  public static isValidNew(newCard: string): boolean {
    return CardUtil.isValid(newCard); //legal
  }
}

// 4 - array any type parameter
function $r(value: string, ...params: any[]): string {
  if (params) {
    return value
  }
  return ''
}
const anyArrayasString = 'any type'
let msg: string = $r('test');
msg = $r('test', anyArrayasString) //legal

let a: number | string = 42;
a++;  //Global variable, error

let a2: number | string = 42;
const b2 = a++;  //Global variable, error
const b3 = --a; //Global variable, error

function foo5() {
    let a: number | string = 42;
    a++;  //local variable，no error
    --a; //local variable，no error
}

let c: number = 45
const fg = c++ // no error only number type

function boo2(a: number | undefined) {
    if (a != undefined) {
        a++;  //function para, no error
    }
}

class ay {
    a: number | string = 42;
}

function boo() {
    let aa: ay = new ay();
    aa.a++;   //object prop，Error
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

function foo7() {
    let aObj: A3 | null = new A3();
    if (aObj != null) {
        aObj.a = 2.0;   //smart type, the type of local variable 'a' is narrowed to 'A' and no longer includes 'null', no error
    }
}

function foo8(bObj: B3 | undefined, ccc: number | undefined) {
    if (bObj != undefined && bObj.b != undefined) {
        bObj.b.a = 2.0; // Error
        bObj.b?.a; // Ok         
        bObj.b.a; // Error
        const a1 = bObj.b?.a; // Ok
        let a2: number = a1; //  Error
        a2 =  bObj.b?.a;	 //  Error
        let a3: number | undefined  = a1; // Ok
        a3 =  bObj.b?.a; // Ok
        const b1 = bObj.b; // Ok
        let b2: A3 = b1; //  Error
        b2 = b1; //  Error
        let b3: A3 | undefined = b1; //  ok
        b3 = b1; //  ok
        const a: A3 | null = bObj.b; // Ok
        const b: A3 = bObj.b; // Error
        const c: A3 = bObj.b as A3; // Ok
        bObj.test1(bObj.b); // Ok
        bObj.test2(bObj.b); // Error
        bObj.test2(bObj.b as A3); // Ok
        bObj.test3(1, bObj.b); // Error
        bObj.test3(1, bObj.b as A3); // Ok

        if (ccc != undefined) { // Ok
            const d: number = ccc; // Ok
        }

        if (B3.bbb != undefined) { // Ok
            const e: number = B3.bbb; // Error
        }
    }
}

function foo9(ccc: number | undefined) {
  if (B3.bbb) { // Ok
		return B3.bbb; // Error
	}
	if (!B3.bbb) { // Ok
		return B3.bbb; // Error
	}
	if (B3.bbb != undefined) { // Ok
		return B3.bbb; // Error
	}
	if (B3.bbb instanceof number) { // Ok
		return B3.bbb; // Error
	}
	if (typeof B3.bbb === 'number') { // Ok
		return B3.bbb; // Error
	}
	if (!ccc) { // Ok
		return ccc; // Ok
	}
	if (ccc != undefined) { // Ok
		return ccc; // Ok
	}
	if (ccc instanceof number) { // Ok
		return ccc; // Ok
	}
	if (typeof ccc === 'number') { // Ok
		return ccc; // Error
	}
}

function foo10() {
    return B3.bbb; // Ok
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
            return this.aaa; //  error,arkts-no-ts-like-smart-type，this.aaa是class内部属性，不支持smart type，需要报错
        }

        if (this.aClass?.a) { // Ok
            this.test1(this.aClass?.a); // Ok
            this.test2(this.aClass?.a); // Ok
            this.test3(this.aClass?.a); // Ok
        }
        return this.instance; // Error
    }

    test1(a?: number) {}
    test2(a: number | undefined) {}
    test3(a: number) {}
}

class D {
    test() {}
}

class E {
    test() {}
}

let varD: D | E = new E();
varD.test(); // Ok

function foo1(aObj: A3 | null) {
    if (aObj != null) {
        aObj.a = 2.0;   //smart type，the type of function parameter 'a' is narrowed to 'A' and no longer includes 'null'. no error
    }
}

function aoo() {
    let bObj = new B3();
    if (bObj.b != null) {
        bObj.b.a = 2.0;   //bObj.b is an object property, which does not conform to smart type, so there is no type narrowing here. error
    }
}

function boo5() {
    let aObj: A3 | null = new A3();
    if (aObj instanceof A3) {
        aObj.a = 2.0;   //smart type，the type of local variable 'a' is narrowed to 'A'，no error
    }
}

function boo1(aObj: A3 | null) {
    if (aObj instanceof A3) {
        aObj.a = 2.0;   //smart type，the type of local variable 'a' is narrowed to 'A' no error
    }
}

function coo() {
    let bObj = new B3();
    if (bObj.b instanceof A3) {
        bObj.b.a = 2.0;   //bObj.b is an object property, which doesn't meet the smart type criteria, so there's no type narrowing here, error
    }
}

class C { }
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
        this.b = this.a.getBB();// // Need to report the error "arkts-no-ts-like-smart-type"
    }
}

class ef {
    a: number = 1.0;
}

function roo(aObj: ef | null) {
    if (typeof aObj === 'ef') {
        aObj.a = 2.0; // Error
    }
}

class Ay {
    private connectOptions?: Promise<void>;

    private connect(): Promise<void> {
        this.connectOptions = new Promise(() => { });
        return this.connectOptions; // Need to report the error "arkts-no-ts-like-smart-type"
    }
}

enum enumA {
    a = 0,
    b = 1,
    c = 2
}

class AF {
    test(a: enumA): void {
        switch (a) {
            case 0:
                return AF.test1(); // No need to report the error "arkts-no-ts-like-smart-type"
            case 1:
                return AF.test2(); // No need to report the error "arkts-no-ts-like-smart-type"
            default:
                return AF.test3(); // No need to report the error "arkts-no-ts-like-smart-type"
        }
    }
    public static test1() { }
    public static test2() { }
    public static test3() { }
}

function $g(value: string, ...params: string[]) { }

(name: string) => {
    $g(name, name); // No need to report the error "arkts-no-ts-like-smart-type"
}

class eq {
    static getStatusBarTypeValue(): number {
        return 1;
    }
    getCurStatusBarTypeValue(): number {
        return eq.getStatusBarTypeValue(); // No need to report the error "arkts-no-ts-like-smart-type"
    }
}

class er {
    static test1(): string { return '1'; }
    static test2(): string { return '2'; }

    main(num: number): string {
        if (num) {
            return er.test1(); // No need to report the error "arkts-no-ts-like-smart-type"
        } else {
            return er.test2(); // No need to report the error "arkts-no-ts-like-smart-type"
        }
    }
}

export enum TestType {
    Image = 'Image',
    Edit = 'Edit',
}
export type EditorTestType = TestType | 'default';

function EnterEditor(tip: TestType | 'default') {
    console.log('test function');
}

function beforeTask(tip: EditorTestType) {
    EnterEditor(tip); //legal
}