## Optional Methods Not Supported

**Rule:** `arkts-optional-methods`

**Severity: error**

In ArkTS1.2, class methods are shared by all instances. Adding support for optional methods would increase the cost of null checks for developers and impact performance.

**ArkTS1.1**

```typescript
interface InterfaceA {
  aboutToDisappear?(): void
}
class ClassA {
  aboutToDisappear?(): void {}
}
```

**ArkTS1.2**

```typescript
interface InterfaceA {
  aboutToDisappear?: () => void
}
class ClassA {
  aboutToDisappear?: () => void = () => {}
}
```