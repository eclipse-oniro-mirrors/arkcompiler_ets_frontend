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

function arrayBuiltinPositive(): void {
  let values = new Array<number>(3);
  values.copyWithin(0, 1, 2);
  values.fill(0, 1, 2);
  values.includes(2, 1);
  values.push(5);
  values.unshift(0);
}

function explicitIntBuiltinArgsPositive(): void {
  let target: int = 0;
  let start: int = 1;
  let end: int = 2;
  let values: Array<number> = [1, 2, 3, 4];
  values.copyWithin(target, start, end);

  let text = 'this is test for builtin api num to int';
  text.substring(1, 2);
  text.split('is', 1);
  String.fromCharCode(1, 2);
}

function builtinReturnUsedAsNumberPositive(): void {
  let values = [1.0, 2.0, 3.0];
  let mapValues = new Map<string, string>();
  let setValues = new Set<string>();
  console.log(values.length + mapValues.size + setValues.size);
}

function typedArrayBuiltinPositive(): void {
  let values = new Uint8Array();
  values.copyWithin(0, 1, 2);
  values.slice(1, 2);
  values.subarray(1, 2);
}

function arrayMethodsWithCallbacksPositive(): void {
  let arr = new Array<string>();
  let intVal: int = 10;
  
  arr.sort((str1: string, str2: string): number => str1.length - str2.length);
  arr.push('bb');
  arr.splice(intVal, 1, 'splice');
  arr.unshift('aa');
  
  arr.findIndex((value: string, index: number, array: Array<string>): boolean => array[index] == value);
  arr.filter((value: string, index: number, array: Array<string>): boolean => array[index] == value);
  arr.find((value: string, index: number, array: Array<string>): boolean => array[index] == value);
  arr.every((value: string, index: number, array: Array<string>): boolean => array[index] == value);
  arr.some((value: string, index: number, array: Array<string>): boolean => array[index] == value);
  
  arr.reduce((previousValue: string, currentValue: string, index: number, array: Array<string>): string => previousValue);
  arr.reduce<number>((previousValue: number, currentValue: string, index: number, array: Array<string>): number => previousValue, intVal);
  arr.reduceRight<number>((previousValue: number, currentValue: string, index: number, array: Array<string>): number => previousValue, intVal);
  arr.map<number>((currentValue: string, index: number, array: Array<string>): number => currentValue.length);
  arr.reduceRight((previousValue: string, currentValue: string, index: number, array: Array<string>): string => previousValue);
  
  let arrLen: number = arr.length;
  console.log(arrLen);
}

function typedArrayAllTypesPositive(): void {
  let int8Arr = new Int8Array();
  let int16Arr = new Int16Array();
  let int32Arr = new Int32Array();
  let float32Arr = new Float32Array();
  let float64Arr = new Float64Array();
  let uint8Arr = new Uint8Array();
  let uint16Arr = new Uint16Array();
  let uint32Arr = new Uint32Array();
  let uint8ClampedArr = new Uint8ClampedArray();
  
  int8Arr.copyWithin(1, 2);
  int16Arr.slice(1);
  int32Arr.indexOf(1);
  float32Arr.lastIndexOf(1, 2);
  float64Arr.fill(1);
  uint8Arr.subarray(1, 2);
  uint16Arr.length;
  uint32Arr.indexOf(1);
  uint8ClampedArr.copyWithin(1, 2);
}

function stringAllMethodsPositive(): void {
  let str = "this is test for builtin api num to int.";
  str.localeCompare(str);
  str.startsWith('is', 1);
  str.endsWith('is', 1);
  str.substring(1);
  str.split('is', 1);
  str.split(new RegExp('this', 'g'), 1);
  str.charCodeAt(1);
  String.fromCharCode(1);
  String.fromCharCode(1, 2);
  str.includes('this is', 1);
  str.padEnd(1);
  str.padEnd(1, 'x');
  str.padStart(1, 'x');
  str.search("this");
  String.fromCodePoint(1);
  String.fromCodePoint(1, 2);
}

function dateAllMethodsPositive(): void {
  let date = new Date();
  let intVal: int = 100;
  
  date.valueOf();
  date.getDate();
  date.setTime(intVal);
  date.getUTCDate();
  date.setUTCDate(10);
  date.getDay();
  date.getUTCDay();
  date.getFullYear();
  date.getUTCFullYear();
  
  date.setFullYear(10);
  date.setFullYear(10, 12);
  date.setUTCFullYear(2026);
  date.setUTCFullYear(2026, 5);
  date.setUTCFullYear(2026, 5, 25);
  
  date.getHours();
  date.getUTCHours();
  date.setHours(22);
  date.setHours(22, 30);
  date.setUTCHours(22, 30, 30);
  date.setUTCHours(22, 30, 30, 500);
  
  date.getMilliseconds();
  Date.UTC(2026, 5);
  Date.UTC(2026, 5, 25, 10, 30, 30, 500);
  date.getUTCMilliseconds();
  date.setMilliseconds(500);
  date.setUTCMilliseconds(500);
  
  date.getSeconds();
  date.getUTCSeconds();
  date.setSeconds(30);
  date.setSeconds(30, 500);
  date.setUTCSeconds(30, 500);
  
  date.getMinutes();
  date.setUTCMinutes(30, 30, 500);
  date.getUTCMinutes();
  date.setMinutes(30, 30, 500);
  
  date.getMonth();
  date.getUTCMonth();
  date.setMonth(5, 25);
  date.setUTCMonth(5, 25);
  
  date.getTime();
  date.setTime(intVal);
  date.getTimezoneOffset();
}

function dateAndMathPositive(): void {
  let date = new Date();
  date.setUTCFullYear(2026, 5, 25);
  Date.UTC(2026, 5, 25);
  console.log(Math.max(1.5, 2.5));
}
