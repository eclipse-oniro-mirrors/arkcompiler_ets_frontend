## 类实现接口时，不能用类方法替代对应interface属性

**规则：**`arkts-no-method-overriding-field`

**级别：error**

ArkTS1.2不支持structural type，属性和方法不能互相转换。

**ArkTS1.1**

```typescript
interface Person {
  cb: () => void
}

class student implements Person{
  cb() {}
} 

interface Transformer<T> {
  transform: (value: T) => T; // 违反规则
}

class StringTransformer implements Transformer<string> {
  transform(value: string) { return value.toUpperCase(); }  // 违反规则
}
```

**ArkTS1.2**

```typescript
interface Person {
  cb(): void
}

class student implements Person{
  cb() {}
}

interface Transformer<T> {
  transform(value: T): T;  // 变成方法
}

class StringTransformer implements Transformer<string> {
  transform(value: string) { return value.toUpperCase(); }  // 正确
}
```
