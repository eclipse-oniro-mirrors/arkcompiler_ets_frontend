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

import { IDataSourcePrefetching } from '@kit.ArkUI';
import DataSource from './limit_void_type_sdk2';
import { prefetchTest as preTest } from './limit_void_type_sdk2';
import { res } from './limit_void_type_sdk2';
import { LocalDataSource } from './LimitedVoidType3';
class DS implements IDataSourcePrefetching {
  cancel(index: number): void | Promise<void> {  //report error
    throw new Error('Method not implemented.');
  }

  totalCount(): number {
    throw new Error('Method not implemented.');
  }

  getData(index: number): number {
    throw new Error('Method not implemented.');
  }

  registerDataChangeListener(): void {
    throw new Error('Method not implemented.');
  }

  unregisterDataChangeListener(): void {
    throw new Error('Method not implemented.');
  }
  prefetch(index: number): void | Promise<void> {  //report error

  }

  prefetch2(index: number): void | Promise<void> {

  }
}
const  ds = new DataSource();
let num:number=ds.totalCount();
function testDataSource(){
  console.log(`${num}=num`);
  ds.cancel(66);   //report error
}
function testPrefetch() {
  preTest();
}

function testCancel(){
  return res;
}

class localDemo{
  dataSource:DS|undefined=undefined;
  constructor() {
    this.dataSource = new DS();
  }
  test(){
    this.dataSource?.getData(111);
    this.dataSource?.registerDataChangeListener();
    this.dataSource?.unregisterDataChangeListener();
    this.dataSource?.cancel(111);  //report error
  }
}
function localTest(){
  return ds.cancel(222);  //report error
}
const lds = new LocalDataSource();
lds.prefetch(999); 
lds.cancel(222);

interface localInterface extends IDataSourcePrefetching {
  prefetch(index: number): void | Promise<void>;
}
