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

import {
  Component,
  State,
  Column,
  Text,
  Row,
  Blank,
  Button,
  PropRef,
} from '@kit.ArkUI';

interface ChildComponentOptions {
  count: number;
}

@Component
struct SuperComponent1 {
  @State countOptions: ChildComponentOptions = { count: 0 }

  build() {
    Column() {
      ChildComponent1({ options1: this.countOptions })
      Text(`${this.countOptions.count}`)
    }
  }
}

@Component
struct SuperComponent2 {
  @State countOptions: ChildComponentOptions = { count: 0 }

  build() {
    Column() {
      ChildComponent2({ options2: this.countOptions })
      Text(`${this.countOptions.count}`)
    }
  }
}

@Component
struct SuperComponent3 {
  @State countOptions: ChildComponentOptions = { count: 0 }

  build() {
    Column() {
      ChildComponent3({ options3: this.countOptions })
      Text(`${this.countOptions.count}`)
    }
  }
}

@Component
struct ChildComponent1 {
  @PropRef options1: ChildComponentOptions;

  build() {
    Row() {
      Text(`${this.options1.count}`)
      Blank()
      Button('+').onClick(() => {
        this.options1.count++;
      })
      Button('-').onClick(() => {
        this.options1.count--;
      })
    }
  }
}

@Component
struct ChildComponent2 {
  @PropRef options2: ChildComponentOptions;

  build() {
    Row() {
      Text(`${this.options2.count}`)
      Blank()
      Button('change1').onClick(() => {
        this.options2.count = 1;
      })
      Button('change2').onClick(() => {
        this.options2.count = 1;
      })
    }
  }
}

@Component
struct ChildComponent3 {
  @PropRef options3: ChildComponentOptions;

  build() {
    Row() {
      Text(`${this.options3.count}`)
      Blank()
      Button('+').onClick(() => {
        ++this.options3.count;
      })
      Button('-').onClick(() => {
        --this.options3.count;
      })
    }
  }
}