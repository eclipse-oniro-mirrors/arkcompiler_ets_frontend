## 重载API变更

**规则：** `sdk-api-static-overload`

**规则解释：**

ArkTS-Sta重载实现方式发生变化，ArkTS-Dyn部分API的名称和使用方式需要变化。

**变更原因：**

为了增强运行时的性能和效率，ArkTS-Sta采用静态化方式实现重载。

**适配建议：**

在调用ArkTS-Dyn相关API时，需同步更新接口名称及参数传递方式。

**示例：**

ArkTS-Dyn

```
// ArkTS-Dyn API定义
on(type: 'update', callback: () => void): void;

// 应用代码
import { pasteboard } from '@kit.BasicServicesKit';

const systemPasteboard: pasteboard.SystemPasteboard = pasteboard.getSystemPasteboard();
let listener = () => {
  console.info('The system pasteboard has changed.');
};
systemPasteboard.on('update', listener);
```

ArkTS-Sta

```
// ArkTS-Sta API定义
onUpdate(callback: UpdateCallback): void

// 应用代码
import { pasteboard } from '@kit.BasicServicesKit';

const systemPasteboard: pasteboard.SystemPasteboard = pasteboard.getSystemPasteboard();
let listener = () => {
  console.info('The system pasteboard has changed.');
};
systemPasteboard.onUpdate(listener);
```