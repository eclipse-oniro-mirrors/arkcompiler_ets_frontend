---
title: 除了默认导出外，所有顶层声明必须添加`declare`
---

规则：除了默认导出外，所有顶层声明必须添加`declare`关键字。

- 示例：
  - `export class A {}` → `export declare class A {}`
  - `export function foo()` → `export declare function foo()`
  - `export default class A {}` → `export default class A {}`
