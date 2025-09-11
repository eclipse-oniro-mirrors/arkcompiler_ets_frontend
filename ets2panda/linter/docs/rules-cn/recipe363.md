## `BuilderNode`的泛型参数不能为元组

**规则：** `arkui-buildernode-generic-no-tuple`

**级别：** error

在ArkTS1.2中，`BuilderNode`的泛型参数不能为元组，需要进行修改。

**ArkTS1.1**

```typescript
import { BuilderNode, NodeController, FrameNode } from '@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode<[]> | null = null;
  public builderNode2: BuilderNode<[Params]> | null = null;
  public frameNode: FrameNode | null = null;

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null) {
      this.builderNode1 = new BuilderNode(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode2 = new BuilderNode<[Params]>(uiContext, { selfIdealSize: { width: 300, height: 200} });
    }
    return this.frameNode;
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import { UIContext } from '@kit.ArkUI';

import { BuilderNode, NodeController, FrameNode } from '@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode | null = null;
  public builderNode2: BuilderNode<Params> | null = null;
  public frameNode: FrameNode | null = null;

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null) {
      this.builderNode1 = new BuilderNode(uiContext, { selfIdealSize : { width: 300.0, height: 200.0} });
      this.builderNode2 = new BuilderNode<Params>(uiContext, { selfIdealSize: { width: 300.0, height: 200.0} });
    }
    return this.frameNode;
  }
}
```