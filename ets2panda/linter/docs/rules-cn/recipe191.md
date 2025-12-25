## 内存默认共享，不提供ASON

**规则：** `arkts-no-need-stdlib-ason`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，对象跨线程传递无需ASON序列化，使用标准的JSON.stringify方法简化数据操作。

**变更原因：**

ArkTS-Sta新增对象天然共享特性，对象跨线程传递无需ASON序列化。

**适配建议：**

将ASON.stringify()方法调用直接更改为JSON.stringify()，并删除ArkTSUtils.前缀。将ASON.parse()方法调用更改为JSON.parse()，并由开发者根据相应的[JSON](../arkts-utils/arkts-json.md) API进行修改。

**示例：**

ArkTS-Dyn

```typescript
import { lang, collections } from '@kit.ArkTS';
import { ArkTSUtils } from '@kit.ArkTS';
let arr = new collections.Array(1, 2, 3);
let str = ArkTSUtils.ASON.stringify(arr);
console.info(str);

// ASON.parse场景
type ISendable = lang.ISendable;
let jsonText = '{"name": "John", "age": 30, "city": "ChongQing"}';
let obj = ArkTSUtils.ASON.parse(jsonText) as ISendable;
console.info((obj as object)?.["name"]);
```

ArkTS-Sta

```typescript
let arr = new Array<number>(1, 2, 3);
let str = JSON.stringify(arr);
console.info(str);

// JSON.parse场景
class Person {
  name: string = "";
  age: number = 0;
  city: string = "";
} 
let jsonText: string = `{"name": "John", "age": 30, "city": "ChongQing"}`;
let typ: Type = Type.of(new Person());
try {
  let result = JSON.parse<Person>(jsonText, typ) as Person;
  console.info(result.name); // John
} catch (error) {
  const err: Error = error as Error;
  console.error(`Failed to parse JSON. Code is ${err.code}, message is ${err.message}`);
}
```