---
title: 声明必须只包含类型信息，不包含实现
---

规则：声明必须只包含类型信息，不包含实现。

- 示例对比：

  - 错误示例：

    ```typescript
    export class A {
      foo() {
        console.info(1);
      } // 错误：包含实现体
    }
    ```

  - 正确示例：

    ```typescript
    export declare class A {
      foo(): void; // 正确：仅包含签名
    }
    ```
