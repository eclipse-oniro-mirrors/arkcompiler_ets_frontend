## 不支持TS装饰器

**规则：** `arkts-no-ts-decorators`

**规则解释：**

ArkTS1.2不支持通过自定义装饰器动态改变类、方法、属性或函数参数。

**变更原因：**
 
由于自定义装饰器需要动态改变类、方法、属性，而ArkTS1.2是静态类型语言，所以不支持自定义装饰器。

**适配建议：**

请参考以下示例修改代码。

**示例1：日志追踪装饰器**
```typescript
// ArkTS1.1代码：
// file1.ts
export function Log(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
  const originalMethod = descriptor.value;
  descriptor.value = function (...args: any[]) {
    console.info(`[LOG] 方法 ${propertyKey} 被调用，参数: ${JSON.stringify(args)}`);
    const result = originalMethod.apply(this, args);
    console.info(`[LOG] 方法 ${propertyKey} 返回: ${result}`);
    return result;
  };
}
// file2.ets
import {Log} from './file1';
@Component
struct MyCounter {
  @State count: number = 0;

  @Log
  increment() {
    this.count++;
    return this.count;
  }

  build() {
    Button(`Count: ${this.count}`)
      .onClick(() => this.increment())
  }
}
```
```typescript
// ArkTS1.2代码：
import { Component, Button, ClickEvent } from '@ohos.arkui.component';
import { State } from '@ohos.arkui.stateManagement';

@Component
struct Counter {
  @State count: number = 0;

  increment() {
    console.info(`[LOG] 方法 increment 被调用，参数: []`);
    this.count++;
    const result = this.count;
    console.info(`[LOG] 方法 increment 返回: ${result}`);
    return result;
  }

  build() {
    Button(`Count: ${this.count}`)
      .onClick((e:ClickEvent) => {this.increment()})
  }
}
```
**示例2：防抖装饰器**
```typescript
// ArkTS1.1代码：
// file1.ts
export function Debounce(delay: number = 300) {
  return function (target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    let timer: number = 0;
    const originalMethod = descriptor.value;
    descriptor.value = function (...args: any[]) {
      if (timer) {
        clearTimeout(timer);
      }
      timer = setTimeout(() => {
        originalMethod.apply(this, args);
        timer = 0;
      }, delay);
    };
  };
}
// file2.ets
import {Debounce} from './file1';
@Component
struct SearchBox {
  @State keyword: string = '';

  @Debounce(500)
  onSearchInput(keyword: string) {
    this.keyword = keyword;
    console.info(`搜索: ${keyword}`);
    // 调用搜索API...
  }

  build() {
    TextField({ placeholder: '搜索...' })
      .onChange((value) => this.onSearchInput(value))
  }
}
```
```typescript
// ArkTS1.2代码：
import { Component, Button } from '@ohos.arkui.component';
import { State } from '@ohos.arkui.stateManagement';

@Component
struct SearchBox {
  @State keyword: string = '';
  private debounceTimer: Int = 0;

  onSearchInput(keyword: string) {
    if (this.debounceTimer) {
      clearTimeout(this.debounceTimer);
    }
    this.debounceTimer = setTimeout(() => {
      this.keyword = keyword;
      console.info(`搜索: ${keyword}`);
      // 调用搜索API...
    }, 500);
  }

  build() {
    // TextField({ placeholder: '搜索...' })
    //   .onChange((value) => {this.onSearchInput(value)})
  }
}
```
**示例3：权限校验装饰器**
```typescript
// ArkTS1.1代码：
// file1.ts
export function RequiresPermission(permission: string) {
  return function (target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    const originalMethod = descriptor.value;
    descriptor.value = function (...args: any[]) {
      if (checkUserPermission(permission)) {  // 自定义权限检查函数
        return originalMethod.apply(this, args);
      } else {
        console.error(`[权限不足] 需要 ${permission} 权限`);
        return null;
      }
    };
  };
}
// file2.ets
import {RequiresPermission} from './file1';

@Component
struct AdminPanel {
  @RequiresPermission('admin')
  deleteUser(userId: string) {
    // 删除用户逻辑...
  }

  build() {
    Button('删除用户')
      .onClick(() => this.deleteUser('123'))
  }
}
```
```typescript
// ArkTS1.2代码：
import { Component, Button, ClickEvent } from '@ohos.arkui.component';
import { State } from '@ohos.arkui.stateManagement';

@Component
struct AdminPanel {
  deleteUser(userId: string) {
    if (!checkUserPermission('admin')) {  // 自定义权限检查函数
      console.error(`[权限不足] 需要 admin 权限`);
      return;
    }
    // 删除用户逻辑...
  }

  build() {
    Button('删除用户')
      .onClick((e:ClickEvent) => {this.deleteUser('123')})
  }
}
```
**示例4：性能监控装饰器**
```typescript
// ArkTS1.1代码：
// file1.ts
export function PerformanceMonitor(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
  const originalMethod = descriptor.value;
  descriptor.value = function (...args: any[]) {
    const start = Date.now();
    const result = originalMethod.apply(this, args);
    const end = Date.now();
    console.info(`[性能] 方法 ${propertyKey} 执行耗时: ${end - start}ms`);
    return result;
  };
}
// file2.ets
import {PerformanceMonitor} from './file1';

@Component
struct DataLoader {
  @PerformanceMonitor
  loadLargeData() {
    // 模拟耗时操作
    let sum = 0;
    for (let i = 0; i < 1000000; i++) {
      sum += i;
    }
    return sum;
  }

  build() {
    Button('加载数据')
      .onClick(() => this.loadLargeData())
  }
}
```
```typescript
// ArkTS1.2代码：
import { Component, Button, ClickEvent } from '@ohos.arkui.component';
import { State } from '@ohos.arkui.stateManagement';
@Component
struct DataLoader {
  loadLargeData() {
    const start = Date.now();
    // 模拟耗时操作
    let sum = 0;
    for (let i = 0; i < 1000000; i++) {
      sum += i;
    }
    const end = Date.now();
    console.info(`[性能] 方法 loadLargeData 执行耗时: ${end - start}ms`);
    return sum;
  }

  build() {
    Button('加载数据')
      .onClick((e:ClickEvent) => {this.loadLargeData()})
  }
}
```
**示例5：自动保存装饰器**
```typescript
// ArkTS1.1代码：
// file1.ts
export function AutoSave(key: string) {
  return function (target: any, propertyKey: string) {
    let value = target[propertyKey];
    
    const getter = () => value;
    const setter = (newVal: any) => {
      value = newVal;
      try {
        console.info(`[自动保存] 键: ${key}, 值: ${JSON.stringify(newVal)}`);
        localStorage.setItem(key, JSON.stringify(newVal));  // 实际项目需使用存储API
      } catch (e) {
        console.error(`[自动保存失败] ${e}`);
      }
    };
    
    Object.defineProperty(target, propertyKey, {
      get: getter,
      set: setter,
      enumerable: true,
      configurable: true
    });
  };
}
// file2.ets
import {AutoSave} from './file1';
@Component
struct Settings {
  @AutoSave('user_settings')
  theme: string = 'light';

  build() {
    Row() {
      Button('切换主题')
        .onClick(() => this.theme = this.theme === 'light' ? 'dark' : 'light')
    }
  }
}
```
```typescript
// ArkTS1.2代码：
import { Component, Button, ClickEvent,Row } from '@ohos.arkui.component';
import { State } from '@ohos.arkui.stateManagement';

@Component
struct Settings {
  @State theme: string = 'light';

  setTheme(newTheme: string) {
    this.theme = newTheme;
    try {
      console.info(`[自动保存] 键: user_settings, 值: ${JSON.stringify(newTheme)}`);
      localStorage.setItem('user_settings', JSON.stringify(newTheme));  // 实际项目需使用存储API
    } catch (e) {
      console.error(`[自动保存失败] ${e}`);
    }
  }

  build() {
    Row() {
      Button('切换主题')
        .onClick((e:ClickEvent) => {this.setTheme(this.theme === 'light' ? 'dark' : 'light')})
    }
  }
}
```