## 不支持tagged templates

**规则：** `arkts-no-tagged-templates`

**规则解释：**

ArkTS1.2不支持Tagged templates（标签模板字符串）。

**变更原因：**

ArkTS1.2规范函数调用方式，支持字符串相加，但不支持Tagged templates（标签模板字符串）。

**适配建议：**

改为函数调用和字符串加法。

**示例：**

**ArkTS1.1**

```typescript
function myTag(strings: TemplateStringsArray, value: string): string {
    return strings[0] + value.toUpperCase() + strings[1];
}

const name = 'john';
const result = myTag`Hello, ${name}!`;
console.log(result);

function formatTag(strings: TemplateStringsArray, first: string, last: string): string {  
    return `${strings[0]}${first.toUpperCase()} ${last.toUpperCase()}${strings[1]}`;
}

const firstName = 'john';
const lastName = 'doe';
const result = formatTag`Hello, ${firstName} ${lastName}!`;  // 违反规则
console.log(result);
```

**ArkTS1.2**

```typescript
function myTagWithoutTemplate(strings: string, value: string): string {
    return strings + value.toUpperCase();
}

const name = 'john';

const part1 = 'Hello, ';
const part2 = '!';
const result = myTagWithoutTemplate(part1, name) + part2;
console.log(result);

function formatWithoutTemplate(greeting: string, first: string, last: string, end: string): string {  
    return greeting + first.toUpperCase() + ' ' + last.toUpperCase() + end;
}

const firstName = 'john';
const lastName = 'doe';
const result = formatWithoutTemplate('Hello, ', firstName, lastName, '!');  // 直接使用函数参数
console.log(result);
```