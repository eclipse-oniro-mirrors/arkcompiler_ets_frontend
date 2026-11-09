---
title: Adding the `declare` Modifier to Top-Level Declarations That Lack It
---

ArkTS-Sta requires every declaration in a declaration file to be an ambient declaration. Therefore, Declgen adds the `declare` modifier to all top-level declarations.

Input:

{{sample:input}}

Output:

{{sample:output}}
