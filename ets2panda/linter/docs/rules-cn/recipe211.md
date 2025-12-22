## 不支持重复case语句

**规则：** `arkts-case-expr`

**规则解释：**

ArkTS1.2不支持Switch语句的中case重复。

**变更原因：**
 
提高代码的可读性。

**适配建议：**

避免出现重复的case。

**示例：**

**ArkTS1.1**

```typescript
const num = 1;
switch (num) {
    case 1:
        console.log('First match');
    case 1:
        console.log('Second match');
        break;
    default:
        console.log('No match');
}

enum Status {
    Active,
    Inactive
}

const state = Status.Active;
switch (state) {
    case Status.Active:
        console.log('User is active');
        break;
    case Status.Active: // 违反规则
        console.log('Already active');
        break;
}
```

**ArkTS1.2**

```typescript
const num = 1;
switch (num) {
    case 1:
        console.log('First match');
        console.log('Second match');
        break;
    default:
        console.log('No match');
}

switch (state) {
    case Status.Active:
        console.log('User is active');
        console.log('Already active'); // 代码合并
        break;
}
```