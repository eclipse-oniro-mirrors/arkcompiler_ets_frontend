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
import url from './api/@ohos.url';

let paramsObj = new url.URLParams('aa=%E4%B8%AD%E5%9B%BD%BD'); //error
console.log(paramsObj.get('aa')) // 中国%BD console.log(paramsObj.toString()) // aa=%E4%B8%AD%E5%9B%BD%25BD"
const paramsObject1 = new url.URLSearchParams('fod=bay&edg=bap');//error
paramsObject1.append('fod', '3');//error

let objectParams = new url.URLSearchParams([ ['user1', 'abc1'], ['query2', 'first2'], ['query3', 'second3'] ]);//error
let objectParams1 = new url.URLSearchParams({"fod" : '1' , "bard" : '2'});//error
let objectParams2 = new url.URLSearchParams('?fod=1&bard=2');//error
let urlObject1 = new url.URL('https://developer.mozilla.org/?fod=1&bard=2');
let params1 = new url.URLSearchParams(urlObject1.search); //error*2

const paramsObject2 = new url.URLSearchParams('fod=bay&edg=bap');//error
paramsObject2.delete('fod');//error

let searchParamsObject1 = new url.URLSearchParams("keyName1=valueName1&keyName2=valueName2");//error
let iter: Iterable<Object[]> = searchParamsObject1.entries();//error
let pairs = Array.from(iter); //BuiltinAll
for (let pair of pairs) { // Show keyName/valueName pairs
  console.log(pair[0]+ ', '+ pair[1]);
}

const myURLObject = new url.URL('https://developer.exampleUrl/?fod=1&bard=2');
myURLObject.searchParams.forEach((value, name, searchParams) => {//error*2
  console.log(name, value, myURLObject.searchParams === searchParams);//error
});

let paramsObject3 = new url.URLSearchParams('name=Jonathan&age=18');//error
let name = paramsObject3.get("name"); //error
let age = paramsObject3.get("age"); //error
let getObj = paramsObject3.get("abc"); //error

let urlObject2 = new url.URL('https://developer.exampleUrl/?fod=1&bard=2');
let params2 = new url.URLSearchParams(urlObject2.search.slice(1));//error*2
params2.append('fod', '3');//error
console.log(params2.getAll('fod').toString())//error
let paramsObject4 = new url.URLSearchParams(urlObject2.search.slice(1));//error*2
paramsObject4.has('bard') === true;//error

let searchParamsObject2 = new url.URLSearchParams("key1=value1&key2=value2"); //error
let keys = Array.from(searchParamsObject2.keys());//error+BuiltinAll
for (let key of keys) {
  console.log(key);
}

let urlObject3 = new url.URL('https://developer.exampleUrl/?fod=1&bard=2');
let paramsObject = new url.URLSearchParams(urlObject3.search.slice(1));//error*2
paramsObject.set('baz', '3');//error

let searchParamsObject = new url.URLSearchParams("c=3&a=9&b=4&d=2");//error
searchParamsObject.sort(); //error
console.log(searchParamsObject.toString());//error
let urlObject = new url.URL('https://developer.exampleUrl/?fod=1&bard=2');
let params = new url.URLSearchParams(urlObject.search.slice(1));//error*2
params.append('fod', '3');//error
console.log(params.toString()); //error

let searchParams = new url.URLSearchParams("key1=value1&key2=value2");//error
let values = Array.from(searchParams.values());//error+BuiltinAll
for (let value of values) { console.log(value); }

//sum:40