## 对象的属性名称必须是有效标识符

**规则：** `sdk-no-literal-as-property-name`

**规则解释：**

ArkTS-Sta中，对象的属性名称必须是有效标识符，不支持使用单引号和连字符。

**变更原因：**

在ArkTS-Sta中，对象的属性名不能使用数字或字符串，以增强对边界场景的约束。

**适配建议：**

将属性名从字符串改为标识符。

**示例：**

ArkTS-Dyn
```typescript
// ArkTS-Dyn API定义
interface RequestHeaders {
    'authorization'?: string;
    'content-type': string;
    'x-custom-header': string
};
// ArkTS-Dyn应用代码
const headers: RequestHeaders = {
    'content-type': 'application/json',
    'x-custom-header': 'custom-value',
    'authorization': 'Bearer your-token-here'
};
```

ArkTS-Sta
```typescript
// ArkTS-Sta API定义
interface RequestHeaders {
    authorization?: string;
    contentType: string;
    xCustomHeader: string
};

// ArkTS-Sta应用代码
const headers: RequestHeaders = {
    contentType: 'application/json', // 必填
    xCustomHeader: 'my-custom-value', // 必填
    authorization: 'Bearer your-token-here'
};
```