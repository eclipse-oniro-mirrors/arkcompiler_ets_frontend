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

export class SHA1Hash{
    private _int32(data:Int32Array,offset?:int):void{
        offset=0
        swap32(data[offset ++] as int)
    }
}
const swap32:(num:int)=>int = ((c:int):int =>c)

/* @@? 19:16 Warning Warning: 'As' expression for cast is deprecated for numeric types. Use explicit conversion function Double.toInt(...) instead. */
