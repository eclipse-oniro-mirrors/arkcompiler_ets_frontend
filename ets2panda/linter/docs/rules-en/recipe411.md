## instanceof Target Cannot Be a Function

**Rule:** `arkts-no-instanceof-func`

**Severity: error**

ArkTS1.2 no longer implements inheritance via prototypes, so there are no prototypes/constructors, and objects cannot be created from arbitrary functions. Therefore, the target of instanceof cannot be a function.

**ArkTS1.1**

```typescript
function foo() {}
function bar(obj: Object) {
  console.info('obj instanceof foo :' ,obj instanceof foo);
}
```

**ArkTS1.2**
```typescript
function bar(obj: Object) {
console.info('obj instanceof foo :', obj instanceof string);
}
```