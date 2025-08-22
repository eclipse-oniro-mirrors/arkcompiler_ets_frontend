## Duplicate case Statements Not Supported

**Rule:** `arkts-case-expr`

**Severity: error**

ArkTS1.2 does not support duplicate case statements in switch blocks to improve code readability.

**ArkTS1.1**

```typescript
const num = 1;
switch (num) {
    case 1:
        console.log('First match');
    case 1:
        console.log('Second match');
        break;
    default:
        console.log('No match');
}

enum Status {
    Active,
    Inactive
}

const state = Status.Active;
switch (state) {
    case Status.Active:
        console.log('User is active');
        break;
    case Status.Active: // Violates the rule
        console.log('Already active');
        break;
}
```

**ArkTS1.2**

```typescript
const num = 1;
switch (num) {
    case 1:
        console.log('First match');
        console.log('Second match');
        break;
    default:
        console.log('No match');
}

switch (state) {
    case Status.Active:
        console.log('User is active');
        console.log('Already active'); // Merge code
        break;
}
```