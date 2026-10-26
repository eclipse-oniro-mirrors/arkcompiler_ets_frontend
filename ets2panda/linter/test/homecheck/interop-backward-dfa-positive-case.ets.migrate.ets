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


export class RecordUtils {
  static objTransToMap(src: object): Map<string, string> {
    let arr: [string, string][] = Object.entries(src);
    return new Map<string, string>();
  }

  static transToMap(src: Record<string, string>): Map<string, string> {
    Object.keys(src).forEach(key => {});
    return new Map();
  }
}
