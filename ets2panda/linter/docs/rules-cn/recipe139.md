## 不支持为函数增加属性

**规则：** `arkts-no-func-props`

**规则解释：**

ArkTS1.2不支持在函数上动态添加属性。

**变更原因：**
 
ArkTS1.2是静态类型语言，不支持在函数，方法上动态增加属性。

**适配建议：**

使用类来封装函数和属性。

**示例：**

**ArkTS1.1**

```typescript
function foo(path: string): void {
  console.log(path)
}
foo.baz = 1

const obj = {
  foo(path: string): void {
    console.log(path);
  }
};
obj.foo.baz = 2; // 违反规则

function createLogger() {
  function log(message: string) {
    console.log(message);
  }
  log.level = "debug"; // 违反规则
  return log;
}

const logger = createLogger();
console.log(logger.level);

function counter() {
  counter.count = (counter.count || 0) + 1; // 违反规则
  return counter.count;
}
console.log(counter());
```

**ArkTS1.2**

```typescript
class T {
  static foo(path: string): void {
    console.log(path)
  }
  static bar: number = 1
}

class T {
  static foo(path: string): void {
    console.log(path);
  }

  static baz: number = 2;
}
T.foo("example");
console.log(T.baz);

class Logger {
  static level = "debug";

  static log(message: string) {
    console.log(message);
  }
}
Logger.log("test");
console.log(Logger.level);

class Counter {
  static count = 0;

  static increment() {
    this.count += 1;
    return this.count;
  }
}
console.log(Counter.increment());
```