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
namespace AA {
    export class B<T>{}
}

let fecthResult = await checkAsyncDelay(async (): Promise< AA.B<string> > => {
    if (!this.photoAccessHelper) {
        return undefined;
    }
    return await this.photoAccessHelper.getAssets(options);
}, 'getPhotoAssetByUri --> getAssets');


/* @@? 19:19 Error Semantic error ESE0303: Type '*ERROR_TYPE*' can not be awaited, it is not a Promise. */
/* @@? 19:25 Error Semantic error ESE0143: Unresolved reference checkAsyncDelay */
/* @@? 19:25 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 19:25 Warning Warning W173979: Await in non-aysnc function is deprecated, please move it to async function. */