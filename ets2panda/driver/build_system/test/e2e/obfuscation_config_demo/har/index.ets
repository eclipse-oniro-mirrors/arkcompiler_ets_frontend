/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
'use static'

export class IndexPage {
  static getMessage(): string {
    return 'Hello from static library';
  }

  static getVersion(): string {
    return '1.0.0';
  }
}

export function harFunction(): void {
  console.log('HAR function called');
}

export class InternalUtils {
  static internalCalculate(a: number, b: number): number {
    console.log(`InternalUtils：计算输入a=${a}, b=${b}`); // 会被移除
    console.warn("InternalUtils：警告日志（不会被移除，-remove-log仅移除无返回值的console.log）");
    return a * b + 10;
  }
}

export class StringUtils {
  private Tag:string = StringUtils.name;
  static concatStr(a: string, b: string): string {
    return `[HAR工具类处理] ${a} + ${b}`;
  }

  static getRandomStr(len: number = 6): string {
    const chars = 'abcdefghijklmnopqrstuvwxyz0123456789';
    return `[HAR随机字符串] ${chars}`;
  }
}

export class Test{
  private TestTag:string = Test.name;
  static test(): void {
    console.log(`[HAR工具类处理] ${Test.name}`);
  }
}