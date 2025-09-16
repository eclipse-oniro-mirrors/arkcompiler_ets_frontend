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
import { rcp } from "@kit.RemoteCommunicationKit";
export type externalRequestHeaders = rcp.RequestHeaders;
export type externalResponseHeaders = rcp.ResponseHeaders;
export interface externalRequestHeadersInterface extends rcp.RequestHeaders {
  customHeader1: string;
}
export interface externalResponseHeadersInterface extends rcp.ResponseHeaders {
  customHeader2: string;
}
export const headers:string[]=[
  'accept-ranges',
  'allow',
  'cache-control',
  'content-encoding',
  'content-range',
  'content-type',
  'date',
  'etag',
  'expires',
  'location',
  'retry-after',
  'set-cookie',
  'server',
  'www-authenticate',
]

export default function getCookie():string{
  return headers[7];
}
export const generateRange = () => 'range';
export let localUpgrade:string = 'upgrade';
export let localRange:string = generateRange();
export class Header{
  acceptLanguage = headers[6];
  cookie = 'cookie';
  getUpgrade(){
    return localUpgrade;
  }
}

export interface internalResponseHeaders<T extends object> {
  rcpHeaders: rcp.ResponseHeaders;
  additionalHeaders: T;
}
interface AdditionalHeaders {
  customHeader: string;
}
type CustomResponseHeaders = internalResponseHeaders<AdditionalHeaders>;
const cusRequestHeaders: CustomResponseHeaders = {
  rcpHeaders: {
    'content-range': 'json', // Error
    server: 'bytes=0-1023' // Error
  },
  additionalHeaders: {
    customHeader: 'custom value'
  }
};
const basicHeaders: rcp.ResponseHeaders = {
  "set-cookie": 'Bearer abc123',
  'retry-after': 'application/json' // Error
};
basicHeaders.allow =''; // Error
function createHeaders(cookie:string,authToken?: string): rcp.ResponseHeaders {
  return {
    server: undefined,
    expires: 'application/json',
    'accept-ranges': 'no-cache', // Error
    'www-authenticate': 'MyApp/2.0' // Error
  };
}
createHeaders(basicHeaders.date!); // Error
function getRange(){
  return basicHeaders?.["content-range"]; // Error
}

export type localResponseHeaders = rcp.ResponseHeaders;
let localHeaders:localResponseHeaders = {
  'content-encoding': 'UTF-8', // Error
  'content-range': 'gzip, deflate', // Error
  'location': 'en-US,en;q=0.9', // Error
  'set-cookie': 'session_id=123456', // Error
  'accept-ranges': 'bytes=0-1023', // Error
  'www-authenticate': 'websocket', // Error
  'content-type':'application/json' // Error
}
class RemoteTest{
  setCookie(cookie:string){
    cookie = 'cookie'
  }
  createHeaders(cookie:string,authToken?: string): rcp.ResponseHeaders {
    return {
      'accept-ranges':  undefined, // Error
      'content-encoding': 'application/json', // Error
      date: '2025', // Error
      "cache-control": 'no',
      allow:'web', // Error
      'www-authenticate':'' // Error
    };
  }
  constructor() {
    createHeaders(basicHeaders.date!); // Error
  }
}