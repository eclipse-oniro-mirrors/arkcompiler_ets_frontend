## 限制使用标准库

**规则：** `arkts-limited-stdlib`

**规则解释：**

ArkTS-Sta中禁止使用以下接口：

- 全局对象的属性和方法：`eval`

- Object： `__proto__`、`__defineGetter__`、`__defineSetter__`、`__lookupGetter__`、`__lookupSetter__`、`assign`、`create`、`defineProperties`、`defineProperty`、`freeze`、`fromEntries`、`getOwnPropertyDescriptor`、`getOwnPropertyDescriptors`、`getOwnPropertySymbols`、`getPrototypeOf`、`hasOwnProperty`、`is`、`isExtensible`、`isFrozen`、`isPrototypeOf`、`isSealed`、`preventExtensions`、`propertyIsEnumerable`、`seal`、`setPrototypeOf`

- Reflect：`apply`、`construct`、`defineProperty`、`deleteProperty`、`getOwnPropertyDescriptor`、`getPrototypeOf`、`isExtensible`、`preventExtensions`、`setPrototypeOf`

- Proxy：`apply`、`construct`、`defineProperty`、`deleteProperty`、`get`、`getOwnPropertyDescriptor`、`getPrototypeOf`、`has`、`isExtensible`、`ownKeys`、`preventExtensions`、`set`、`setPrototypeOf`

**变更原因：**
 
ArkTS-Sta不允许使用这些受限的Stdlib API，这些接口大多与动态特性相关。

**适配建议：**

不使用这些受限的Stdlib API。