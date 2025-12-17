### Object-getOwnPropertyNames方法变更

**规则：** `arkts-builtin-object-getOwnPropertyNames`

**ArkTS-Dyn版本签名：**  
  `static getOwnPropertyNames(o: any): string[]`

**参数：**
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | `o` | `any` | 是 | 要获取属性名的对象。 |

**返回值：**
  | 类型 | 说明 |
  | -------- | -------- |
  | `string[]` | 对象的属性名数组。 |

**示例：**  
  ```typescript
  class C {
    a: number = 1;
    b: number = 2;
  }
  const a = new C();
  Object.getOwnPropertyNames(a);
  ```

**ArkTS-Sta版本签名：**  
  不支持。

**适配建议：** 
  使用`Object.keys`代替。