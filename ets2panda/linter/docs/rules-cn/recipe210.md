## 限定switch语句中case语句类型

**规则：** `arkts-switch-expr`

**规则解释：**

ArkTS-Sta的switch表达式类型只能为number、string、enum。

**变更原因：**
 
提高代码可读性和执行性能。

**适配建议：**

使用number、string、enum作为switch表达式类型。

**示例：**

ArkTS-Dyn

```typescript
let isTrue: boolean = Boolean(1);
switch (isTrue) {
  case true:
    console.info('It\'s true');
    break;
  case false:
    console.info('It\'s false');
    break;
}

interface IObj {
  value: number
}

const obj: IObj = { value: 1 };
switch (obj) {
  case { value: 1 } as IObj:
    console.info('Matched');
    break;
}

const arr = [1, 2, 3];
switch (arr) {
  case [1, 2, 3]:
    console.info('Matched');
    break;
}
```

ArkTS-Sta

```typescript
const isTrue = 'true';
switch (isTrue) {
  case 'true':
    console.info('It\'s true');
    break;
  case 'false':
    console.info('It\'s false');
    break;
}

const objValue = 1; // 仅存储值
switch (objValue) {
  case 1:
    console.info('Matched');
    break;
}

const arrValue = '1,2,3'; // 变成字符串
switch (arrValue) {
  case '1,2,3':
    console.info('Matched');
    break;
}
```