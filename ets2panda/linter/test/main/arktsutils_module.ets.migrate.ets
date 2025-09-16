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
import { utils } from './oh_modules/@arkts.utils';

import { utils as ArkTSUtilsAlias } from './oh_modules/@arkts.utils';

import { utils as kitArkTSUtils } from './oh_modules/@kit.ArkTS';

import { utils as definedArkTSUtils } from 'user_defined_worker'; //legal

export { utils } from './oh_modules/@arkts.utils';

function tesCollectionsUsage() {

  const utils1: string = JSON.stringify(1);

  const utils2 = JSON.stringify(1);

  const utils3 = JSON.stringify(1);

  const utils4: string = JSON.stringify(1);

  type CreatedType = ArkTSUtils.ASON.SomeType;

  const someType: CreatedType = ArkTSUtils.ASON.SomeType;

  const map: number = ArkTSUtils.ASON.ParseReturnType.map;
}
