## 不支持编译阶段根据PromiseSettledResult的status值确定其实际类型

**规则：** arkts-not-support-PromiseSettledResult-smartcast

**级别：** error

arkts1.2中不支持对类的成员变量进行智能转换（智能类型差异arkts-no-ts-like-smart-type）。

**智能转换：** 编译器会在某些场景下（如instanceof、null检查、上下文推导等）识别出对象的具体类型，自动将变量转换为相应类型，而无需手动转换。

**ArkTS1.1**
```typescript
let f1 = Promise.resolve<string>('fulfilled 1');
Promise.allSettled<string>([f1])
  .then((results: PromiseSettledResult<string>[]) => {
    results.forEach((result: PromiseSettledResult<string>) => {
      if (result.status === 'fulfilled') {
        console.log(`${result.value} `);
      } else {
        console.log(`${result.reason.message} `);
      }
    });
  })
```

**ArkTS1.2**
```typescript
let f1 = Promise.resolve<string>('fulfilled 1');
Promise.allSettled<string>([f1])
  .then((results: PromiseSettledResult<string>[]) => {
    results.forEach((result: PromiseSettledResult<string>) => {
      if (result.status === 'fulfilled') {
          let result1 = result as PromiseFulfilledResult<string>;
          console.log(result1.value);
            } else {
             let result1 = result as PromiseRejectedResult;
             console.log(result1.reason.message);
            }
      });
   })
```
