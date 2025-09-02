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
import { rcp } from '@kit.RemoteCommunicationKit';
class LI implements rcp.WriteFile {
  write(buffer: ArrayBuffer): Promise<number | void> {
    throw new Error()
  }
}

const sp:rcp.IncomingDataCallback = (incomingData) => {
  console.log('');
}
export  default class MyDataSource implements IDataSourcePrefetching {
  constructor() {}
  prefetch(index: number): void | Promise<void> {   //report error
    throw new Error('MyDataSource Method not implemented.');
  }

  cancel(index: number): void | Promise<void> {  //report error
    throw new Error('MyDataSource Method not implemented.');
  }

  totalCount(): number {
    throw new Error('MyDataSource Method not implemented.');
  }

  getData(index: number): number {
    throw new Error('MyDataSource Method not implemented.');
  }

  registerDataChangeListener(listener?: DataChangeListener): void {
    throw new Error('MyDataSource Method not implemented.');
  }

  unregisterDataChangeListener(listener?: DataChangeListener): void {
    throw new Error('MyDataSource Method not implemented.');
  }
}
const ds = new MyDataSource();
export function prefetchTest(){
  return new MyDataSource().prefetch(666);  //report error
}
function prefetchTest2(){
  return ds.prefetch(666);  //report error
}

export const res = new MyDataSource().cancel(222);  //report error
export const res2 = ds.cancel(222);  //report error

interface BackPressInterceptor {
  onBackPress: ()=> boolean | void;
}

class A implements BackPressInterceptor{
  onBackPress() : boolean | void {
    return false;
  }
}