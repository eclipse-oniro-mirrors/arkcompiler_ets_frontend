---
title: Declarations Must Contain Type Information Only, Without Implementations
---

Rule: a declaration must contain type information only, without any implementation.

- Comparison of examples:

  - Incorrect example:

    ```typescript
    export class A {
      foo() {
        console.info(1);
      } // Error: contains an implementation body
    }
    ```

  - Correct example:

    ```typescript
    export declare class A {
      foo(): void; // Correct: signature only
    }
    ```
