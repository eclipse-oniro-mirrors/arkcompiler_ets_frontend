## 函数类型转换及兼容原则

**规则：** `arkts-incompatible-function-types`

**规则解释：**

当函数类型返回void时，ArkTS-Dyn可返回任意类型，而ArkTS-Sta只能返回void类型。

**变更原因：**
 
对于函数类型转换，ArkTS-Dyn和ArkTS-Sta都遵循参数逆变和返回类型协变的规则。有关逆变和协变的详细解释，请参见[逆变和协变](#逆变和协变)。

而当函数类型返回void时，由于ArkTS-Dyn与ArkTS-Sta中void类型的变化，ArkTS-Sta仅支持返回void类型。详细情况请参考[限制void类型的使用场景](#限制void类型的使用场景)。

**适配建议：**

当函数类型返回void时，实现代码也要返回void。

**示例：**

ArkTS-Dyn

```typescript
type F = () => void;
// 可返回任意类型
let f: F = (): number => {
  return 0;
}
```

ArkTS-Sta

```typescript
type F = () => void;
// 改为相同的返回类型
let f: F = (): void => {};
```