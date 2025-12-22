## arguments Object Not Supported

**Rule:** `arkts-no-arguments-obj`

**Severity:** error

ArkTS1.2 performs strict parameter checks during function calls, so there is no need to use the arguments mechanism to retrieve parameters within functions.

**ArkTS1.1**
```typescript
function foo(u: string) {
  console.log(arguments[0]);
}

function bar(a: number, b?: number) {
  if (arguments.length === 1) {
    console.log("Only one argument passed");
  }
}

function sum() {
  let total = 0;
  for (let i = 0; i < arguments.length; i++) {
    total += arguments[i];
  }
  return total;
}

function test() {
  console.log(arguments.callee);
}
```

**ArkTS1.2**
```typescript
function foo(u: string) {
  console.log(u);
}

function bar(a: number, b?: number) {
  if (b === undefined) {
    console.log("Only one argument passed");
  }
}

function sum(...args: number[]) {  
  // Use `...rest` instead of `arguments`
  return args.reduce((acc, num) => acc + num, 0);
}

function test() {
  console.log(test);  // Use the function name directly
}
```