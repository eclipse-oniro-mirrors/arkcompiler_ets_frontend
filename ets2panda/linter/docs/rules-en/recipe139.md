## Adding Properties to Functions Not Supported

**Rule:** `arkts-no-func-props`

**Severity: error**

ArkTS1.2 does not support dynamically adding properties to functions.

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
obj.foo.baz = 2; // Violates the rule

function createLogger() {
  function log(message: string) {
    console.log(message);
  }
  log.level = "debug"; // Violates the rule
  return log;
}

const logger = createLogger();
console.log(logger.level);

function counter() {
  counter.count = (counter.count || 0) + 1; // Violates the rule
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