---
title: Adding `declare` to All Top-Level Declarations Except the Default Export
---

Rule: every top-level declaration must carry the `declare` keyword, except the default export.

- Examples:
  - `export class A {}` → `export declare class A {}`
  - `export function foo()` → `export declare function foo()`
  - `export default class A {}` → `export default class A {}`
