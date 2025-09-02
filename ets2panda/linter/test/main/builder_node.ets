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

import { NodeController, BuilderNode, FrameNode } from './ui_modules/@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

interface CustomInterface {
  item: string;
  age: number;
}

class CustomClass {
  private item: string = 'C';
  age: number = 10;
}

@Builder
function buildNode(param: Params) {}

function getObject1() {
  const customInterfaceParams: CustomInterface | number =  1 > 0 ? { item: 'C', age: 100 } : 1;
  return customInterfaceParams;
}

function getObject2() {
  const customInterfaceParams: CustomInterface = {
    item: 'C',
    age: 100
  };
  return customInterfaceParams;
}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode<[Params]> | null = null; // error
  public builderNode2: BuilderNode<[Params]> | null = null; // error
  public builderNode3: BuilderNode<[]> | null = null; // error
  public builderNode4: BuilderNode<[]> | null = null; // error
  public frameNode: FrameNode | null = null;
  public item: string = "";

  getObject3() {
    const customInterfaceParams: CustomInterface = {
      item: 'C',
      age: 100
    };
    return customInterfaceParams;
  }

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null
      || this.builderNode3 == null || this.builderNode4 == null) {
      this.builderNode1 = new BuilderNode(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode2 = new BuilderNode<[Params]>(uiContext, { selfIdealSize: { width: 300, height: 200} }); // error
      this.builderNode3 = new BuilderNode(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode4 = new BuilderNode<[]>(uiContext, { selfIdealSize : { width: 300, height: 200} }); // error
      this.builderNode1.build(wrapBuilder<[Params]>(buildNode), new Params(this.item), { nestingBuilderSupported: false }) // error
      let flag = true;
      this.builderNode2.build(wrapBuilder<[Params]>(buildNode), new Params(this.item), { nestingBuilderSupported: flag }); // error
    }

    return this.frameNode;
  }

  updateItem(item: string, customParam: boolean): void {
    this.item = item;
    if (this.builderNode1 && this.builderNode2 && this.builderNode3 && this.builderNode4) {
      if (customParam) {
        const a1: CustomInterface = {
          item: 'C',
          age: 100
        };
        const a2: CustomInterface | number = 1 > 0 ? { item: 'C', age: 100 } : 1;
        const a3 = getObject1();
        const a4 = getObject2();
        const customClassParams: CustomClass = new CustomClass();
        this.builderNode1.update(a1); // error
        this.builderNode2.update(a2); // error
        this.builderNode3.update(a3); // error
        this.builderNode4.update(a4); // error
        this.builderNode1.update({ item: 'C', age: 100 }); // error
        this.builderNode2.update(1 > 0 ? a1 : 1); // error
        this.builderNode3.update(customClassParams);
        this.builderNode3.update({ item: 'C', age: 100}); // error
        this.builderNode4.update(getObject1()); // error
        this.builderNode4.update(getObject2()); // error
        this.builderNode4.update(this.getObject3()); // error
      } else {}
    }
  }
}

@Reusable
@Component
struct ReusableChildComponent {
  private controller: MyNodeController = new MyNodeController();

  aboutToReuse(params: Record<string, object>): void {
    this.controller?.builderNode1?.reuse(params);
  }

  build() {

  }
}