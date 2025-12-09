## `BuilderNode`的`update`接口的入参不能是字面量

**规则：** `arkui-buildernode-update-no-literal`

**级别：** error

在ArkTS1.2中，`BuilderNode`的`update`接口的入参不能是字面量。需要将字面量替换为新建`BuilderNode`节点时泛型中指定的类的实例，并确保该实例具有和字面量相同的字段值。

**ArkTS1.1**

```typescript
import { NodeController, BuilderNode, FrameNode } from '@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

interface CustomInterface {
  item: string;
}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode<[Params]> | null = null;
  public builderNode2: BuilderNode<[Params]> | null = null;
  public builderNode3: BuilderNode<[Params]> | null = null;
  public frameNode: FrameNode | null = null;

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null || this.builderNode3 == null) {
      this.builderNode1 = new BuilderNode<[Params]>(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode2 = new BuilderNode<[Params]>(uiContext, { selfIdealSize : { width: 300, height: 200} });
      this.builderNode3 = new BuilderNode<[Params]>(uiContext, { selfIdealSize : { width: 300, height: 200} });
    }

    return this.frameNode;
  }

  updateItem(item: string, customParam: boolean): void {
    if (this.builderNode1 && this.builderNode2 && this.builderNode3) {
      if (customParam) {
        const customInterfaceParams: CustomInterface = { item: 'C' };
        this.builderNode1!.update(customInterfaceParams);
        this.builderNode2!.update({ item: 'C' });
        this.builderNode3!.update({ item: item });
      } else {
      }
    }
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import { UIContext } from '@kit.ArkUI';

import { NodeController, BuilderNode, FrameNode } from '@kit.ArkUI';

class Params {
  item: string = '';

  constructor(item: string) {
    this.item = item;
  }
}

interface CustomInterface {
  item: string;
}

class MyNodeController extends NodeController {
  public builderNode1: BuilderNode<Params> | null = null;
  public builderNode2: BuilderNode<Params> | null = null;
  public builderNode3: BuilderNode<Params> | null = null;
  public frameNode: FrameNode | null = null;

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.builderNode1 == null || this.builderNode2 == null || this.builderNode3 == null) {
      this.builderNode1 = new BuilderNode<Params>(uiContext, { selfIdealSize : { width: 300.0, height: 200.0} });
      this.builderNode2 = new BuilderNode<Params>(uiContext, { selfIdealSize : { width: 300.0, height: 200.0} });
      this.builderNode3 = new BuilderNode<Params>(uiContext, { selfIdealSize : { width: 300.0, height: 200.0} });
    }

    return this.frameNode;
  }

  updateItem(item: string, customParam: boolean): void {
    if (this.builderNode1 && this.builderNode2 && this.builderNode3) {
      if (customParam) {
        const customInterfaceParams: CustomInterface = { item: 'C' };
        this.builderNode1!.update(new Params(customInterfaceParams.item));
        this.builderNode2!.update(new Params('C'));
        this.builderNode3!.update(new Params(item));
      } else {
      }
    }
  }
}
```