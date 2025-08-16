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

import { ConnectOptions, PersistenceV2, Type } from './ui_modules/@kit.ArkUI';
import { contextConstant } from '@kit.AbilityKit';

PersistenceV2.notifyOnError((key: string, reason: string, msg: string) => {})

@ObservedV2
class SampleChild {
  @Trace childId: number = 0;
  groupId: number = 1;
}

@ObservedV2
class Sample {
  @Type(SampleChild)
  @Trace father: SampleChild = new SampleChild();
}

@Entry
@ComponentV2
struct TestCase2 {
  @Local p1: Sample | undefined = PersistenceV2.globalConnect({type: Sample, defaultCreator:() => new Sample()}); // error

  @Local p2: Sample | undefined = PersistenceV2.globalConnect({type: Sample, key: 'global1', defaultCreator:() => new Sample(), areaMode: contextConstant.AreaMode.EL1}); // error

  options: ConnectOptions<Sample> = {type: Sample, key: 'global2', defaultCreator:() => new Sample()};
  @Local p3: Sample | undefined = PersistenceV2.globalConnect(this.options); // error

  @Local p4: Sample | undefined = PersistenceV2.globalConnect({type: Sample, key: 'global1', defaultCreator:() => new Sample(), areaMode: 3}); // error

  @Local p5: Sample | undefined = PersistenceV2.connect(Sample, () => new Sample()); // error

  build() {
  }
}