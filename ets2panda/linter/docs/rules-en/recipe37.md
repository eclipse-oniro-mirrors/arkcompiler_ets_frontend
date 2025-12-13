## Regular Expressions Not Supported

**Rule:** `arkts-no-regexp-literals`

**Severity: error**

ArkTS1.2 does not support regular expression literals.

**ArkTS1.1**

```typescript
let regex: RegExp = /bc*d/;
let regex = /\d{2,4}-\w+/g; // Violates the rule
function matchPattern(str: string) {
  return str.match(/hello\s+world/i); // Violates the rule
}

let text = "Hello world!";
let result = text.replace(/world/, "ArkTS"); // Violates the rule

let items = "apple,banana, cherry".split(/\s*,\s*/); // Violates the rule
```

**ArkTS1.2**

```typescript
let regex: RegExp =  new RegExp('bc*d');
let regex = new RegExp('\\d{2,4}-\\w+', 'g'); // Use `RegExp` class
function matchPattern(str: string) {
  let regex = new RegExp('hello\\s+world', 'i'); // Use `RegExp` class
  return str.match(regex);
}

let text = "Hello world!";
let regex = new RegExp('world'); // Use `RegExp` class
let result = text.replace(regex, "ArkTS");

let regex = new RegExp('\\s*,\\s*'); // Use `RegExp` class
let items = "apple,banana, cherry".split(regex);
```