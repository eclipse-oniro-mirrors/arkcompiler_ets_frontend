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

export class RecordUtils {
    static getRecord<T>(obj: Record<string, Object>, key: string, df: Record<string, T> = {}): Record<string, T> {
        if (obj == null) return df;
        try {
            if (obj[key] as Record<string, T>) {
                return obj[key] as Record<string, T>;
            }
        } catch (e) {
            console.log(`111`)
        }
        return df;
    }
}