## Enum不可以通过索引访问成员

**规则：** `arkts-enum-no-props-by-index`

**规则解释：**

ArkTS1.2强化枚举静态类型约束（运行时保留类型信息），禁止通过索引访问以替代ArkTS1.1的动态对象行为。

**变更原因：**

1. ArkTS1.1已对索引访问元素的语法做了限制，ArkTS1.2进一步增强了对枚举场景的约束。具体内容请参考[不支持通过索引访问字段](typescript-to-arkts-migration-guide.md#不支持通过索引访问字段)。

2. 在ArkTS1.1上，枚举是动态对象；而在ArkTS1.2上，枚举是静态类型，并具有运行时类型，因此对索引访问做了限制以提高性能。

**适配建议：**

通过枚举的API来实现对应功能。

**示例：**

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

TEST.A;          // 使用点操作符或者enum的值
TEST.A.getName();  // 使用enum对应的方法获取enum的key
```