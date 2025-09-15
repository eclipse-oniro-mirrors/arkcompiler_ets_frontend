## Restricted Types in switch Statements

**Rule:** `arkts-switch-expr`

**Severity: error**

In ArkTS1.2, switch expressions can only be of type number, string, or enum.

**ArkTS1.1**

```typescript
const isTrue = true;
switch (isTrue) {
    case true: // Violates the rule
        console.log('It\'s true'); break;
    case false:  // Violates the rule
        console.log('It\'s false'); break;
}

const obj = { value: 1 };
switch (obj) {  // Violates the rule
    case { value: 1 }:
        console.log('Matched'); break;
}

const arr = [1, 2, 3];
switch (arr) {  // Violates the rule
    case [1, 2, 3]: 
        console.log('Matched'); break;
}
```

**ArkTS1.2**

```typescript
const isTrue = 'true';
switch (isTrue) {
    case 'true': 
        console.log('It\'s true'); break;
    case 'false': 
        console.log('It\'s false'); break;
}

const objValue = 1;  // Store only the value
switch (objValue) {
    case 1:
        console.log('Matched'); break;
}

const arrValue = '1,2,3';  // Convert to a string
switch (arrValue) {
    case '1,2,3':
        console.log('Matched'); break;
}
```