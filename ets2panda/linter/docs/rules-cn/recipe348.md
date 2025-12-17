### String-构造函数变更为invoke方法

**规则：** `arkts-builtin-cotr`

**ArkTS-Dyn版本签名：**  
  `(value?: any): string`

**参数：**
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | value | any | 否 | 任意类型值，默认值为空。 |

**返回值：**
  | 类型 | 说明 |
  | -------- | -------- |
  | string | 字符串表示。 |

**示例：**  
  ```typescript
  function create(ctor: StringConstructor) {
    return ctor({});
  }
  ```

**ArkTS-Sta版本签名：**  
  `static String.invoke(value?: Object | undefined | null): String`

**参数：**
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | value | Object \| undefined \| null | 否 | 限定类型值，默认值为空。 |

**返回值：**
  | 类型 | 说明 |
  | -------- | -------- |
  | String | 字符串对象。 |

**示例：**  
  ```typescript
  new String({}); // 需确保参数符合类型
  ```

**适配建议：** 
  不要使用Constructor类型，使用invoke或new的方式直接创建对象。

## Symbol

**变更梗概**

- [iterator符号属性移除](#iterator符号属性移除)

**变更详情**