## Tagged Templates Not Supported

**Rule:** `arkts-no-tagged-templates`

**Severity**

ArkTS1.2 standardizes function call syntax, supporting string concatenation but not tagged template literals.

**ArkTS1.1**

```typescript
function myTag(strings: TemplateStringsArray, value: string): string {
    return strings[0] + value.toUpperCase() + strings[1];
}

const name = 'john';
const result = myTag`Hello, ${name}!`;
console.log(result);

function formatTag(strings: TemplateStringsArray, first: string, last: string): string {  
    return `${strings[0]}${first.toUpperCase()} ${last.toUpperCase()}${strings[1]}`;
}

const firstName = 'john';
const lastName = 'doe';
const result = formatTag`Hello, ${firstName} ${lastName}!`;  // Violates the rule
console.log(result);
```

**ArkTS1.2**

```typescript
function myTagWithoutTemplate(strings: string, value: string): string {
    return strings + value.toUpperCase();
}

const name = 'john';

const part1 = 'Hello, ';
const part2 = '!';
const result = myTagWithoutTemplate(part1, name) + part2;
console.log(result);

function formatWithoutTemplate(greeting: string, first: string, last: string, end: string): string {  
    return greeting + first.toUpperCase() + ' ' + last.toUpperCase() + end;
}

const firstName = 'john';
const lastName = 'doe';
const result = formatWithoutTemplate('Hello, ', firstName, lastName, '!');  // Direct function parameters
console.log(result);
```