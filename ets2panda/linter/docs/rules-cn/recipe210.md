## 限定switch语句中case语句类型

**规则：** `arkts-switch-expr`

**规则解释：**

ArkTS1.2的switch表达式类型只能为number、string、enum。

**变更原因：**
 
提高代码可读性和执行性能。

**适配建议：**

使用number、string、enum作为switch表达式类型。

**示例：**

**ArkTS1.1**

```typescript
const isTrue = true;
switch (isTrue) {
    case true: // 违反规则
        console.log('It\'s true'); break;
    case false:  // 违反规则
        console.log('It\'s false'); break;
}

const obj = { value: 1 };
switch (obj) {  // 违反规则
    case { value: 1 }:
        console.log('Matched'); break;
}

const arr = [1, 2, 3];
switch (arr) {  // 违反规则
    case [1, 2, 3]: 
        console.log('Matched'); break;
}
```

**ArkTS1.2**

```typescript
const isTrue = 'true';
switch (isTrue) {
    case 'true': 
        console.log('It\'s true'); break;
    case 'false': 
        console.log('It\'s false'); break;
}

const objValue = 1;  // 仅存储值
switch (objValue) {
    case 1:
        console.log('Matched'); break;
}

const arrValue = '1,2,3';  // 变成字符串
switch (arrValue) {
    case '1,2,3':
        console.log('Matched'); break;
}
```