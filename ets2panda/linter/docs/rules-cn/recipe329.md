## Enum不可以通过索引访问成员

**规则：**`arkts-enum-no-props-by-index`

**级别：error**

1. ArkTS1.1上已对索引访问元素的语法做了限制，ArkTS1.2对枚举场景增强约束。具体内容请参考[不支持通过索引访问字段](typescript-to-arkts-migration-guide.md#不支持通过索引访问字段)。

2. ArkTS1.1上枚举是动态对象，ArkTS1.2是静态类型，枚举具有运行时类型。为获得更高的性能，对[]访问做了限制。

**ArkTS1.1**

```typescript
enum TEST {
  A,
  B,
  C
}

TEST['A'];       // ArkTS1.2上不支持这种语法
TEST[0];    // ArkTS1.2上不支持这种语法
```

**ArkTS1.2**

```typescript
enum TEST {
  A,
  B,
  C
}

TEST.A;          // 使用.操作符或者enum的值
TEST.A.getName();  // 使用enum对应的方法获取enum的key
```
