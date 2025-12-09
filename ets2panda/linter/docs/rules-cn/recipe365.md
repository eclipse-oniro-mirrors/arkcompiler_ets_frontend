## 不支持`nestingBuilderSupported`属性

**规则：** `arkui-buildernode-no-nestingbuildersupported`

**级别：** error

在ArkTS1.2中，不支持`nestingBuilderSupported`属性。如果`BuilderNode`中`build`接口的入参中包含该属性，需要按照示例进行修改。

**ArkTS1.1**

```typescript
import { NodeController, BuilderNode, FrameNode } from '@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

@Builder
function buildNode(param: Params) {}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode<[Params]> | null = null;
  public builderNode2: BuilderNode<[Params]> | null = null;
  public frameNode: FrameNode | null = null;
  public item: string = "";
  public flag: boolean = true;

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null) {
      this.builderNode1 = new BuilderNode<[Params]>(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode2 = new BuilderNode<[Params]>(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode1!.build(wrapBuilder(buildNode), new Params(this.item), { nestingBuilderSupported: false });
      this.builderNode2!.build(wrapBuilder(buildNode), new Params(this.item), { nestingBuilderSupported: this.flag });
    }

    return this.frameNode;
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  Builder,
  UIContext,
  wrapBuilder,
} from '@kit.ArkUI';

import { NodeController, BuilderNode, FrameNode } from '@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

@Builder
function buildNode(param: Params) {}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode<Params> | null = null;
  public builderNode2: BuilderNode<Params> | null = null;
  public frameNode: FrameNode | null = null;
  public item: string = "";
  public flag: boolean = true;

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null) {
      this.builderNode1 = new BuilderNode<Params>(uiContext, { selfIdealSize : { width: 300.0, height: 200.0} });
      this.builderNode2 = new BuilderNode<Params>(uiContext, { selfIdealSize : { width: 300.0, height: 200.0} });
      this.builderNode1!.build(wrapBuilder(buildNode), new Params(this.item), {});
      this.builderNode2!.build(this.flag ? wrapBuilder(buildNode) : wrapBuilder(buildNode), new Params(this.item), {});
    }

    return this.frameNode;
  }
}
```