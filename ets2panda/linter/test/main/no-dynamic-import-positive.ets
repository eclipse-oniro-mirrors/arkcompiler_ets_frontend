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

import { Data } from './file';

import * as Shapes from './shapes';

import Utils from './utils';

import type { EllipseShape } from '../api/@ohos.arkui.shape';

import ZipUtil from 'create-zip-file';

function getZip() {
  return ZipUtil.create();
}

import Logger from './logger';

Logger.log("hello");

import { A } from './a';
import { B } from './b';

const v = A.value + B.value;