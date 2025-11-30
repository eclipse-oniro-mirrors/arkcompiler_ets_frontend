/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

import {worker} from '@kit.ArkTS'; // sdk-type-query report in real env
// // test1
import sendablePhotoAccessHelper from '@ohos.file.sendablePhotoAccessHelper' // 报错 PASS

// test2
import mySendablePhotoAccessHelper from '@ohos.file.sendablePhotoAccessHelper' // 报错 PASS

import { dataSharePredicates } from '@kit.ArkData';
import { photoAccessHelper } from '@kit.MediaLibraryKit';
import spHelper from '@ohos.file.sendablePhotoAccessHelper'; // Error-sendable不再支持

worker.parentPort.self;

const self = worker.parentPort.self;

console.log(worker.parentPort.self)

const parentPort = worker.parentPort;
parentPort.self.name;
parentPort.self.clearTimeout;

const workerport = worker.workerPort;
workerport.self.name;
workerport.self.clearTimeout;

let sHelper = spHelper.getPhotoAccessHelper(globalThis.abilityContext);
let predicates: dataSharePredicates.DataSharePredicates = new dataSharePredicates.DataSharePredicates();
let fetchOptions: photoAccessHelper.FetchOptions = {
  fetchColumns: [],
  predicates: predicates
};
// Array<SharedPhotoAsset>
export const sharedPhotoAssetList = sHelper.getSharedPhotoAssets(fetchOptions);
sharedPhotoAssetList.forEach((asset, index) => {
  let pt = asset.position // Error-position:PositionType不再可用-未识别
  let ps = asset.subtype // Error-subtype:PhotoSubtype不再可用-未识别
  let aa = asset.movingPhotoEffectMode // Error-movingPhotoEffectMode:MovingPhotoEffectMode 不再可用-未识别
  let bb = asset.dynamicRangeType // Error-dynamicRangeType:DynamicRangeType 不再可用-未识别
  let cc = asset.thumbnailVisible // Error-thumbnailVisible:ThumbnailVisibility 不再可用-未识别
});