## 不支持`this.value!!`形式的双向绑定

**规则：** `arkui-no-!!-bidirectional-data-binding`

### 系统组件参数双向绑定

在ArkTS1.2中，不支持`this.value!!`形式的双向绑定。对于系统组件参数的双向绑定，应改为`$$(this.value)`的形式。

**ArkTS1.1**

```typescript
@Entry
@ComponentV2
struct Index {
  @Local text: string = '';

  build() {
    Column() {
      Text(this.text)
      TextInput({text: this.text!!})
        .width(300)
    }.width('100%').height('100%')
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  Entry,
  ComponentV2,
  Local,
  Column,
  Text,
  TextInput,
  $$,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  @Local text: string = '';

  build() {
    Column() {
      Text(this.text)
      TextInput({text: $$(this.text)})
        .width(300)
    }.width('100%').height('100%')
  }
}
```

### 自定义组件间双向绑定

在ArkTS1.2中，对于自定义组件间的双向绑定，要将原来的双向绑定语法糖展开。

**ArkTS1.1**

```typescript
@Entry
@ComponentV2
struct Index {
  @Local value: number = 0;

  build() {
    Column() {
      Text(`${this.value}`)
      Button(`change value`).onClick((e: ClickEvent) => {
        this.value++;
      })
      Star({ value: this.value!! })
    }
  }
}

@ComponentV2
struct Star {
  @Param value: number = 0;
  @Event $value: (val: number) => void = (val: number) => {};

  build() {
    Column() {
      Text(`${this.value}`)
      Button(`change value `).onClick((e: ClickEvent) => {
        this.$value(10);
      })
    }
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  Entry,
  ComponentV2,
  Local,
  Column,
  Text,
  Button,
  ClickEvent,
  Param,
  Event,
} from '@kit.ArkUI';

@Entry
@ComponentV2
struct Index {
  @Local value: number = 0;

  build() {
    Column() {
      Text(`${this.value}`)
      Button(`change value`).onClick((e: ClickEvent) => {
        this.value++;
      })
      // 在ArkTS1.2中，展开双向绑定语法糖
      Star({
        value: this.value,
        $value: (value: number) => {
            this.value = value;
        }
      })
    }
  }
}

@ComponentV2
struct Star {
  @Param value: number = 0;
  @Event $value: (val: number) => void = (val: number) => {};

  build() {
    Column() {
      Text(`${this.value}`)
      Button(`change value `).onClick((e: ClickEvent) => {
        this.$value(10);
      })
    }
  }
}
```