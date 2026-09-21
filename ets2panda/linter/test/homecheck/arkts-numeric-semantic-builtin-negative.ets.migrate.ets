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

function arrayConstructorAndFields(): void {
  let arraySize: number = 3; // arkts-builtin-api-num2int
  let values = new Array<number>(arraySize);

  let valuesLength: number = values.length; // arkts-builtin-api-num2int
  let copied = new Array<number>(valuesLength);
  console.log(copied.length);
}

function arrayMethodArgsAndReturns(): void {
  let values: Array<number> = [1, 2, 3, 4];
  let target: number = 0;
  let start: number = 1;
  let end: number = 2;
  values.copyWithin(target, start, end);
  values.fill(0, start, end);
  values.includes(2, start);

  let found: number = values.findIndex((value: number): boolean => value > 1); // arkts-builtin-api-num2int
  let fromLast: number = values.lastIndexOf(2, end);
  let pushedLength: number = values.push(5); // arkts-builtin-api-num2int
  let unshiftedLength: number = values.unshift(0); // arkts-builtin-api-num2int
  console.log(found + fromLast + pushedLength + unshiftedLength);
}

function arrayMethodsWithCallbacks(): void {
  let arr = new Array<string>();
  let num1: number = 10; // arkts-builtin-api-num2int
  
  let arr2 = arr.sort((str1: string, str2: string): number => str1.length - str2.length); // arkts-builtin-api-num2int
  let puNum: number = arr.push('bb'); // arkts-builtin-api-num2int
  let spliced = arr.splice(num1, 1.1, 'splice'); // arkts-builtin-api-num2int
  let unshiftNum: number = arr.unshift(); // arkts-builtin-api-num2int
  
  let findIndexNum: number = arr.findIndex((value: string, index: number, array: Array<string>): boolean => array[index] == value); // arkts-builtin-api-num2int
  let filtered = arr.filter((value: string, index: number, array: Array<string>): boolean => array[index] == value); // arkts-builtin-api-num2int
  let foundStr = arr.find((value: string, index: number, array: Array<string>): boolean => array[index] == value); // arkts-builtin-api-num2int
  let everyResult = arr.every((value: string, index: number, array: Array<string>): boolean => array[index] == value); // arkts-builtin-api-num2int
  let someResult = arr.some((value: string, index: number, array: Array<string>): boolean => array[index] == value); // arkts-builtin-api-num2int
  
  let reducedStr = arr.reduce((previousValue: string, currentValue: string, index: number, array: Array<string>): string => previousValue); // arkts-builtin-api-num2int
  let reducedNum: number = arr.reduce<number>((previousValue: number, currentValue: string, index: number, array: Array<string>): number => previousValue, num1); // arkts-builtin-api-num2int
  let reducedRightNum: number = arr.reduceRight<number>((previousValue: number, currentValue: string, index: number, array: Array<string>): number => previousValue, num1); // arkts-builtin-api-num2int
  let mapped = arr.map<number>((currentValue: string, index: number, array: Array<string>): number => currentValue.length); // arkts-builtin-api-num2int
  let reducedRightStr = arr.reduceRight((previousValue: string, currentValue: string, index: number, array: Array<string>): string => previousValue); // arkts-builtin-api-num2int
  
  let arrLen: number = arr.length; // arkts-builtin-api-num2int
  console.log(arrLen + puNum + unshiftNum + findIndexNum + reducedNum + reducedRightNum);
}

function typedArrayMethodArgsAndReturns(): void {
  let values = new Uint8Array();
  let target: number = 0; // arkts-builtin-api-num2int
  let start: number = 1; // arkts-builtin-api-num2int
  let end: number = 2; // arkts-builtin-api-num2int
  values.copyWithin(target, start, end);
  values.slice(start, end);
  values.subarray(start, end);

  let found: number = values.indexOf(start, target); // arkts-builtin-api-num2int
  let typedLength: number = values.length; // arkts-builtin-api-num2int
  console.log(found + typedLength);
}

function typedArrayAllTypesRepresentative(): void {
  let int8Arr = new Int8Array();
  let int16Arr = new Int16Array();
  let int32Arr = new Int32Array();
  let float32Arr = new Float32Array();
  let float64Arr = new Float64Array();
  let bigInt64Arr = new BigInt64Array();
  let uint8Arr = new Uint8Array();
  let uint16Arr = new Uint16Array();
  let uint32Arr = new Uint32Array();
  let uint8ClampedArr = new Uint8ClampedArray();
  
  let num1: number = 1.0; // arkts-builtin-api-num2int
  let num2: number = 2.1; // arkts-builtin-api-num2int
  
  int8Arr.copyWithin(num1, num2); // arkts-builtin-api-num2int
  int16Arr.slice(num1); // arkts-builtin-api-num2int
  int32Arr.indexOf(num1); // arkts-builtin-api-num2int
  float32Arr.lastIndexOf(num1, num2); // arkts-builtin-api-num2int
  float64Arr.fill(num1); // arkts-builtin-api-num2int
  bigInt64Arr.indexOf(100n, num1); // arkts-builtin-api-num2int
  uint8Arr.subarray(num1, num2); // arkts-builtin-api-num2int
  uint16Arr.length; // arkts-builtin-api-num2int
  uint32Arr.indexOf(num1); // arkts-builtin-api-num2int
  uint8ClampedArr.copyWithin(num1, num2); // arkts-builtin-api-num2int
}

function arrayBufferByteLength(): void {
  let arrBuffer = new ArrayBuffer(100);
  let byteLen: number = arrBuffer.byteLength; // arkts-builtin-api-num2int
  console.log(byteLen);
}

function stringMethodArgsAndReturns(): void {
  let text = 'this is test for builtin api num to int';
  let start: number = 1; // arkts-builtin-api-num2int
  let end: number = 4; // arkts-builtin-api-num2int
  text.startsWith('is', start);
  text.endsWith('is', end);
  text.substring(start, end);
  text.split('is', start);
  text.charCodeAt(start);
  String.fromCharCode(start, end);
  text.padStart(end, 'x');

  let searchIndex: number = text.search('builtin'); // arkts-builtin-api-num2int
  let copied = new Array<number>(searchIndex);
  console.log(copied.length);
}

function stringMoreMethods(): void {
  let str = "this is test for builtin api num to int.";
  let num1: number = 1.0; // arkts-builtin-api-num2int
  let compareResult: number = str.localeCompare(str); // arkts-builtin-api-num2int
  str.startsWith('is', num1); // arkts-builtin-api-num2int
  str.endsWith('is', num1); // arkts-builtin-api-num2int
  str.substring(num1); // arkts-builtin-api-num2int
  str.split('is', num1); // arkts-builtin-api-num2int
  str.split(new RegExp('this', 'g'), num1); // arkts-builtin-api-num2int
  str.charCodeAt(num1); // arkts-builtin-api-num2int
  String.fromCharCode(num1); // arkts-builtin-api-num2int
  String.fromCharCode(num1, num1); // arkts-builtin-api-num2int
  str.includes('this is', num1); // arkts-builtin-api-num2int
  str.padEnd(num1); // arkts-builtin-api-num2int
  str.padEnd(num1, 'x'); // arkts-builtin-api-num2int
  str.padStart(num1, 'x'); // arkts-builtin-api-num2int
  let searchResult: number = str.search("this"); // arkts-builtin-api-num2int
  String.fromCodePoint(num1); // arkts-builtin-api-num2int
  String.fromCodePoint(num1, num1); // arkts-builtin-api-num2int
  console.log(compareResult + searchResult);
}

function dateMethodArgsAndReturns(): void {
  let date = new Date();
  let year: number = 2026; // arkts-builtin-api-num2int
  let month: number = 5; // arkts-builtin-api-num2int
  let day: number = 25; // arkts-builtin-api-num2int
  date.setUTCFullYear(year, month, day);
  Date.UTC(year, month, day);

  let currentDay: number = date.getUTCDate(); // arkts-builtin-api-num2int
  let copied = new Array<number>(currentDay);
  console.log(copied.length);
}

function dateAllMethodsRepresentative(): void {
  let date = new Date();
  let num: number = 100.0; // arkts-builtin-api-num2int
  
  let valueNum: number = date.valueOf(); // arkts-builtin-api-num2int
  let dateNum: number = date.getDate(); // arkts-builtin-api-num2int
  let setTimeNum: number = date.setTime(num); // arkts-builtin-api-num2int
  let utcDateNum: number = date.getUTCDate(); // arkts-builtin-api-num2int
  date.setUTCDate(valueNum); // arkts-builtin-api-num2int
  let dayNum: number = date.getDay(); // arkts-builtin-api-num2int
  let utcDayNum: number = date.getUTCDay(); // arkts-builtin-api-num2int
  let fullYearNum: number = date.getFullYear(); // arkts-builtin-api-num2int
  let utcFullYearNum: number = date.getUTCFullYear(); // arkts-builtin-api-num2int
  
  date.setFullYear(dateNum); // arkts-builtin-api-num2int
  date.setFullYear(dateNum, 12); // arkts-builtin-api-num2int
  date.setFullYear(dateNum, 12.1); // arkts-builtin-api-num2int
  date.setUTCFullYear(2026.1); // arkts-builtin-api-num2int
  date.setUTCFullYear(2026, 5.0); // arkts-builtin-api-num2int
  date.setUTCFullYear(2026, 5, 25.5); // arkts-builtin-api-num2int
  
  let hoursNum: number = date.getHours(); // arkts-builtin-api-num2int
  let utcHoursNum: number = date.getUTCHours(); // arkts-builtin-api-num2int
  date.setHours(22.1); // arkts-builtin-api-num2int
  date.setHours(num, num); // arkts-builtin-api-num2int
  date.setUTCHours(num, num, num); // arkts-builtin-api-num2int
  date.setUTCHours(num, num, num, num); // arkts-builtin-api-num2int
  
  let millisecondsNum: number = date.getMilliseconds(); // arkts-builtin-api-num2int
  Date.UTC(num, num); // arkts-builtin-api-num2int
  Date.UTC(num, num, num); // arkts-builtin-api-num2int
  Date.UTC(num, num, num, num, num, num, num); // arkts-builtin-api-num2int
  let utcMillisecondsNum: number = date.getUTCMilliseconds(); // arkts-builtin-api-num2int
  date.setMilliseconds(num); // arkts-builtin-api-num2int
  date.setUTCMilliseconds(num); // arkts-builtin-api-num2int
  
  let secondsNum: number = date.getSeconds(); // arkts-builtin-api-num2int
  let utcSecondsNum: number = date.getUTCSeconds(); // arkts-builtin-api-num2int
  date.setSeconds(num); // arkts-builtin-api-num2int
  date.setSeconds(num, num); // arkts-builtin-api-num2int
  date.setUTCSeconds(num, num); // arkts-builtin-api-num2int
  
  let minutesNum: number = date.getMinutes(); // arkts-builtin-api-num2int
  date.setUTCMinutes(num, num, num); // arkts-builtin-api-num2int
  let utcMinutesNum: number = date.getUTCMinutes(); // arkts-builtin-api-num2int
  date.setMinutes(num, num, num); // arkts-builtin-api-num2int
  
  let monthNum: number = date.getMonth(); // arkts-builtin-api-num2int
  let utcMonthNum: number = date.getUTCMonth(); // arkts-builtin-api-num2int
  date.setMonth(num, num); // arkts-builtin-api-num2int
  date.setUTCMonth(num, num); // arkts-builtin-api-num2int
  
  let timeNum: number = date.getTime(); // arkts-builtin-api-num2int
  date.setTime(num); // arkts-builtin-api-num2int
  let timezoneOffsetNum: number = date.getTimezoneOffset(); // arkts-builtin-api-num2int
  console.log(valueNum + dateNum + setTimeNum + utcDateNum + dayNum + utcDayNum + fullYearNum + utcFullYearNum + 
              hoursNum + utcHoursNum + millisecondsNum + utcMillisecondsNum + secondsNum + utcSecondsNum + 
              minutesNum + utcMinutesNum + monthNum + utcMonthNum + timeNum + timezoneOffsetNum);
}

function mathMapSetReturns(): void {
  let absoluteInput: number = -3;
  let absoluteValue = Math.abs(absoluteInput); // arkts-builtin-api-num2int
  console.log(absoluteValue);

  let mapValues = new Map<string, string>();
  mapValues.set('first', 'value');
  let mapSize: number = mapValues.size; // arkts-builtin-api-num2int
  let mapBuckets = new Array<number>(mapSize);
  console.log(mapBuckets.length);

  let setValues = new Set<string>();
  setValues.add('first');
  let setSize: number = setValues.size; // arkts-builtin-api-num2int
  let setBuckets = new Array<number>(setSize);
  console.log(setBuckets.length);
}