## Default Parameters Must Follow Required Parameters

**Rule:** `arkts-default-args-behind-required-args`

**Severity: error**

Default parameters before required parameters are meaningless. In ArkTS1.1, calling such an interface still requires passing every default parameter.

**ArkTS1.1**

```typescript
function add(left: number = 0, right: number) { 
  return left + right;
}
```

**ArkTS1.2**

```typescript
function add(left: number, right: number) {
  return left + right;
}
```