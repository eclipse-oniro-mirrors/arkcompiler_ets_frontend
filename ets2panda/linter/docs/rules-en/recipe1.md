## Enum Keys Cannot Be Strings

**Rule:** `arkts-identifiers-as-prop-names`

**Severity: error**

ArkTS1.2 does not support using strings as property or element names for class, interface, or enum. Identifiers must be used instead.

**ArkTS1.1**

```typescript
enum A{
 'red' = '1'
}
```

**ArkTS1.2**

```typescript
enum A{
  red = '1'
}
```