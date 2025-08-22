## arkts-limited-stdlib

**规则：** `arkts-limited-stdlib`

**规则解释：**

ArkTS1.2中禁止使用以下接口：

- 全局对象的属性和方法：`eval`

- Object： `__proto__`、`__defineGetter__`、`__defineSetter__`、`__lookupGetter__`、`__lookupSetter__`、`assign`、`create`、`defineProperties`、`defineProperty`、`freeze`、`fromEntries`、`getOwnPropertyDescriptor`、`getOwnPropertyDescriptors`、`getOwnPropertySymbols`、`getPrototypeOf`、`hasOwnProperty`、`is`、`isExtensible`、`isFrozen`、`isPrototypeOf`、`isSealed`、`preventExtensions`、`propertyIsEnumerable`、`seal`、`setPrototypeOf`

- Reflect：`apply`、`construct`、`defineProperty`、`deleteProperty`、`getOwnPropertyDescriptor`、`getPrototypeOf`、`isExtensible`、`preventExtensions`、`setPrototypeOf`

- Proxy：`handler.apply()`、`handler.construct()`、`handler.defineProperty()`、`handler.deleteProperty()`、`handler.get()`、`handler.getOwnPropertyDescriptor()`、`handler.getPrototypeOf()`、`handler.has()`、`handler.isExtensible()`、`handler.ownKeys()`、`handler.preventExtensions()`、`handler.set()`、`handler.setPrototypeOf()`

**变更原因：**
 
ArkTS1.2不允许使用TypeScript或JavaScript标准库中的这些接口，这些接口大多与动态特性相关。

**适配建议：**

NA