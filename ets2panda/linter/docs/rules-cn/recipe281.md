## 不支持`@Prop`、`@StorageProp`和`@LocalStorageProp`装饰器

**规则：** `arkui-no-prop-decorator`、`arkui-no-storageprop-decorator`、`arkui-no-localstorageprop-decorator`

**规则解释：**

在ArkTS-Sta中，不支持`@Prop`、`@StorageProp`和`@LocalStrorageProp`装饰器，要分别用`@PropRef`、`@StoragePropRef`和`@LocalStroragePropRef`装饰器替代。

**变更原因：**

在ArkTS-Sta中，`@Prop`、`@StorageProp`和`@LocalStrorageProp`装饰器被废弃。

**适配建议：**

分别用`@PropRef`、`@StoragePropRef`和`@LocalStroragePropRef`装饰器去替代`@Prop`、`@StorageProp`和`@LocalStrorageProp`装饰器。

**示例：**

ArkTS-Dyn

```typescript
class User {
  name: string = "";
  age: number = 0;
}

@Entry
@Component
struct FatherComponent {
  @Prop user1: User = new User();
  @StorageLink("user2") user2: User = new User();
  @LocalStorageLink("user3") user3: User = new User();

  build() {
  }
}

@Component
struct ChildComponent {
  @StorageProp("user2") user2: User = new User();
  @LocalStorageProp("user3") user3: User = new User();

  build() {
  }
}
```

ArkTS-Sta

```typescript
'use static'
import {
  Observed,
  Entry,
  Component,
  PropRef,
  StorageLink,
  LocalStorageLink,
  StoragePropRef,
  LocalStoragePropRef,
} from '@kit.ArkUI';

class User {
  name: string = "";
  age: number = 0.0;
}

@Entry
@Component
struct FatherComponent {
  @PropRef user1: User = new User();
  @StorageLink("user2") user2: User = new User();
  @LocalStorageLink("user3") user3: User = new User();

  build() {
  }
}

@Component
struct ChildComponent {
  @StoragePropRef("user2") user2: User = new User();
  @LocalStoragePropRef("user3") user3: User = new User();

  build() {
  }
}
```