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

import { NodeController } from '@kit.ArkUI';

import { typeNode, XComponentOptions, UIContext } from './sdk/FrameNode';
import drawing from './sdk/@ohos.graphics.drawing';
import appManager from './sdk/@ohos.app.ability.appManager';

class MyNodeController extends NodeController {
  makeNode(uiContext: UIContext): void {
    let text = typeNode.createTextNode(uiContext); //createTextNode

    let options: XComponentOptions = {};
    let xcomponent = typeNode.createXComponentNodeWithOptions(uiContext, options); //createXComponentNodeWithOptions

    let column = typeNode.createGridNode(uiContext);

    typeNode.getTextAttribute(uiContext) //getTextAttribute

  }
}

let xDivs: Array<number> = [1, 2, 4];
let yDivs: Array<number> = [1, 2, 4];
let colorArray: Array<number> = [0xffffffff, 0x44444444];
let lattice = drawing.Lattice.createImageLatticeWithArrayInt(xDivs, yDivs, 3, 3, null, null, colorArray); //createImageLatticeWithArrayInt


// onApplicationStateChange
const onAsc = appManager.onApplicationStateChange(applicationStateObserver); //onApplicationStateChange

// onApplicationStateChange - bundleName
let bundleNameList = ['bundleName1', 'bundleName2'];
const onAscBn = appManager.onApplicationStateChange(applicationStateObserver, bundleNameList); //onApplicationStateChange



