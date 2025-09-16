## enum的key不能是字符串

**规则：**`arkts-identifiers-as-prop-names`

**级别：error**

ArkTS1.2不支持将字符串作为class、interface、enum等属性或元素的名称，需要使用标识符来表示。

**ArkTS1.1**

```typescript
enum A{
 'red' = '1'
}
```

**ArkTS1.2**

```typescript
enum A{
  red = '1'
}
```
