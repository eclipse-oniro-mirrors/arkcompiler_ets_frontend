## 不支持`LocalStorage.prop`、`LocalStorage.setAndProp`、`AppStorage.prop`和`AppStorage.setAndProp`接口

**规则：** `arkui-no-prop-function`、`arkui-no-setandprop-function`

**规则解释：**

在ArkTS-Sta中，不支持`LocalStorage.prop`、`LocalStorage.setAndProp`、`AppStorage.prop`和`AppStorage.setAndProp`接口，要分别用`LocalStorage.ref`、`LocalStorage.setAndRef`、`AppStorage.ref`和`AppStorage.setAndRef`接口替代。

**变更原因：**

在ArkTS-Sta中，`LocalStorage.prop`、`LocalStorage.setAndProp`、`AppStorage.prop`和`AppStorage.setAndProp`接口被废弃。

**适配建议：**

分别用`LocalStorage.ref`、`LocalStorage.setAndRef`、`AppStorage.ref`和`AppStorage.setAndRef`接口去替代`LocalStorage.prop`、`LocalStorage.setAndProp`、`AppStorage.prop`和`AppStorage.setAndProp`接口。

**示例：**

ArkTS-Dyn

```typescript
AppStorage.setOrCreate('PropA', 47);
let prop1: SubscribedAbstractProperty<number> = AppStorage.setAndProp<number>('PropA', 48);
let prop2: SubscribedAbstractProperty<number> = AppStorage.prop<number>('PropA');

let storage: LocalStorage = new LocalStorage();
storage.setOrCreate('PropB', 17);
let prop3: SubscribedAbstractProperty<number> = storage.setAndProp<number>('PropB', 18);
let prop4: SubscribedAbstractProperty<number> = storage.prop<number>('PropB');

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('Test')
    }
  }
}
```

ArkTS-Sta

```typescript
'use static'
import {
  AppStorage,
  AbstractProperty,
  LocalStorage,
  Entry,
  Component,
  Column,
  Text,
} from '@kit.ArkUI';

AppStorage.setOrCreate('PropA', 47);
let prop1: AbstractProperty<number> = AppStorage.setAndRef<number>('PropA', 48);
let prop2: AbstractProperty<number> | undefined = AppStorage.ref<number>('PropA');

let storage: LocalStorage = new LocalStorage();
storage.setOrCreate('PropB', 17);
let prop3: AbstractProperty<number> = storage.setAndRef<number>('PropB', 18);
let prop4: AbstractProperty<number> | undefined = storage.ref<number>('PropB');

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('Test')
    }
  }
}
```