## 不支持正则表达式字面量

**规则：** `arkts-no-regexp-literals`

**规则解释：**

ArkTS-Sta不支持正则表达式字面量。

**变更原因：**
 
ArkTS-Sta是静态类型语言，不支持正则表达式字面量，使用严格的类型来定义正则。

**适配建议：**

使用RegExp类代替正则表达式字面量。

**示例：**

ArkTS-Dyn

```typescript
let regex1: RegExp = /bc*d/;
let regex2 = /\d{2,4}-\w+/g; // 违反规则
function matchPattern(str: string) {
  return str.match(/hello\s+world/i); // 违反规则
}

let text = "Hello world!";
let result = text.replace(/world/, "ArkTS"); // 违反规则

let items = "apple,banana, cherry".split(/\s*,\s*/); // 违反规则
```

ArkTS-Sta

```typescript
let regex1: RegExp = new RegExp('bc*d');
let regex2 = new RegExp('\\d{2,4}-\\w+', 'g'); // 使用 `RegExp` 类
function matchPattern(str: string) {
  let regex = new RegExp('hello\\s+world', 'i'); // 使用 `RegExp`
  return str.match(regex);
}

let text = "Hello world!";
let regex3 = new RegExp('world'); // 使用 `RegExp` 类
let result = text.replace(regex3, "ArkTS");

let regex4 = new RegExp('\\s*,\\s*'); // 使用 `RegExp`
let items = "apple,banana, cherry".split(regex4);
```