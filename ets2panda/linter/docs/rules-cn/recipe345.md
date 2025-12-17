### Map-forEach方法签名变更

**规则：** `arkts-builtin-thisArgs`

**ArkTS-Dyn版本签名：**  
  `forEach(callbackfn: (value: V, key: K, map: Map<K, V>) => void, thisArg?: any): void`

**参数：**
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | callbackfn | function | 是 | 遍历函数。 |
  | thisArg | any | 否 | 执行callback时使用的this值，默认值为undefined。 |

callbackfn函数参数说明：
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | value | V | 是 | 当前遍历的键值对中的值。 |
  | key | K | 是 | 当前遍历的键值对中的键。 |
  | map | Map\<K, V\> | 是 | 调用的原始数组。 |


**示例：**  
  ```typescript
  const m: Map<string, string> = new Map<string, string>();
  class Ctx {
    log(key: string, value: string) {
      console.info(key, value);
    }
  }
  m.forEach((value: string, key: string, map: Map<string, string>) => {
    this.log(key, value); // this无法在独立函数中使用
  }, new Ctx());
  ```

**ArkTS-Sta版本签名：**  
  `forEach(callbackfn: (value: V, key: K, map: Map<K, V>) => void): void`

**参数：**
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | callbackfn | function | 是 | 遍历函数。 |

callbackfn函数参数说明：
  | 参数名 | 类型 | 必填 | 说明 |
  | -------- | -------- | -------- | -------- |
  | value | V | 是 | 当前遍历的键值对中的值。 |
  | key | K | 是 | 当前遍历的键值对中的键。 |
  | map | Map\<K, V\> | 是 | 调用的原始Map。 |


**示例：**  
  ```typescript
  const m: Map<string, string> = new Map<string, string>();
  m.forEach((value: string, key: string, map: Map<string, string>) => {
    console.info("value=", value, "key=", key);
  });
  ```

**适配建议：** 
  使用闭包替代thisArg参数。

## WeakMap