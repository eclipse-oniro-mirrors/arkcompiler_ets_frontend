/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export const cookBookMsg: string[] = [];
export const cookBookTag: string[] = [];

/**
 * @note If the value contains multiple parentheses groups,
 *       the rule name must be placed in the last group.
 *
 * @example
 * // Correct (rule name in last parentheses):
 * 'cookBookTag[352] = "1.2 Void conflict...(sdk-ability-asynchronous-lifecycle)"'
 *
 */

cookBookTag[1] =
  'Objects with property names that are not identifiers are not supported (arkts-identifiers-as-prop-names)';
cookBookTag[2] = '"Symbol()" API is not supported (arkts-no-symbol)';
cookBookTag[3] = 'Private "#" identifiers are not supported (arkts-no-private-identifiers)';
cookBookTag[4] = 'Use unique names for types and namespaces. (arkts-unique-names)';
cookBookTag[5] = 'Use "let" instead of "var" (arkts-no-var)';
cookBookTag[6] = '';
cookBookTag[7] = '';
cookBookTag[8] = 'Use explicit types instead of "any", "unknown" (arkts-no-any-unknown)';
cookBookTag[9] = '';
cookBookTag[10] = '';
cookBookTag[11] = '';
cookBookTag[12] = '';
cookBookTag[13] = '';
cookBookTag[14] = 'Use "class" instead of a type with call signature (arkts-no-call-signatures)';
cookBookTag[15] = 'Use "class" instead of a type with constructor signature (arkts-no-ctor-signatures-type)';
cookBookTag[16] = 'Only one static block is supported (arkts-no-multiple-static-blocks)';
cookBookTag[17] = 'Indexed signatures are not supported (arkts-no-indexed-signatures)';
cookBookTag[18] = '';
cookBookTag[19] = 'Use inheritance instead of intersection types (arkts-no-intersection-types)';
cookBookTag[20] = '';
cookBookTag[21] = 'Type notation using "this" is not supported (arkts-no-typing-with-this)';
cookBookTag[22] = 'Conditional types are not supported (arkts-no-conditional-types)';
cookBookTag[23] = '';
cookBookTag[24] = '';
cookBookTag[25] = 'Declaring fields in "constructor" is not supported (arkts-no-ctor-prop-decls)';
cookBookTag[26] = '';
cookBookTag[27] = 'Construct signatures are not supported in interfaces (arkts-no-ctor-signatures-iface)';
cookBookTag[28] = 'Indexed access types are not supported (arkts-no-aliases-by-index)';
cookBookTag[29] = 'Indexed access is not supported for fields (arkts-no-props-by-index)';
cookBookTag[30] = 'Structural typing is not supported (arkts-no-structural-typing)';
cookBookTag[31] = '';
cookBookTag[32] = '';
cookBookTag[33] = '';
cookBookTag[34] = 'Type inference in case of generic function calls is limited (arkts-no-inferred-generic-params)';
cookBookTag[35] = '';
cookBookTag[36] = '';
cookBookTag[37] = 'RegExp literals are not supported (arkts-no-regexp-literals)';
cookBookTag[38] =
  'Object literal must correspond to some explicitly declared class or interface (arkts-no-untyped-obj-literals)';
cookBookTag[39] = '';
cookBookTag[40] = 'Object literals cannot be used as type declarations (arkts-no-obj-literals-as-types)';
cookBookTag[41] = '';
cookBookTag[42] = '';
cookBookTag[43] = 'Array literals must contain elements of only inferrable types (arkts-no-noninferrable-arr-literals)';
cookBookTag[44] = '';
cookBookTag[45] = '';
cookBookTag[46] = 'Use arrow functions instead of function expressions (arkts-no-func-expressions)';
cookBookTag[47] = '';
cookBookTag[48] = '';
cookBookTag[49] = '';
cookBookTag[50] = 'Class literals are not supported (arkts-no-class-literals)';
cookBookTag[51] = 'Classes cannot be specified in "implements" clause (arkts-implements-only-iface)';
cookBookTag[52] = 'Reassigning object methods is not supported (arkts-no-method-reassignment)';
cookBookTag[53] = 'Only "as T" syntax is supported for type casts (arkts-as-casts)';
cookBookTag[54] = 'JSX expressions are not supported (arkts-no-jsx)';
cookBookTag[55] = 'Unary operators "+", "-" and "~" work only on numbers (arkts-no-polymorphic-unops)';
cookBookTag[56] = '';
cookBookTag[57] = '';
cookBookTag[58] = '';
cookBookTag[59] = '"delete" operator is not supported (arkts-no-delete)';
cookBookTag[60] = '"typeof" operator is allowed only in expression contexts (arkts-no-type-query)';
cookBookTag[61] =
  'The bitwise inversion gives different result for "Infinity" (arkts-distinct-infinity-bitwise-inversion)';
cookBookTag[62] = 'Index of tuple must be non-negative integer (arkts-limited-tuple-index-type)';
cookBookTag[63] = '';
cookBookTag[64] = '';
cookBookTag[65] = '"instanceof" operator is partially supported (arkts-instanceof-ref-types)';
cookBookTag[66] = '"in" operator is not supported (arkts-no-in)';
cookBookTag[67] = '';
cookBookTag[68] = '';
cookBookTag[69] = 'Destructuring assignment is not supported (arkts-no-destruct-assignment)';
cookBookTag[70] = '';
cookBookTag[71] = 'The comma operator "," is supported only in "for" loops (arkts-no-comma-outside-loops)';
cookBookTag[72] = '';
cookBookTag[73] = '';
cookBookTag[74] = 'Destructuring variable declarations are not supported (arkts-no-destruct-decls)';
cookBookTag[75] = 'Use string-literal keys with Record (arkts-obj-literal-key-type)';
cookBookTag[76] = 'Type of parameter must be defined explicitly (arkts-require-func-arg-type)';
cookBookTag[77] = 'Template string type is not supported (arkts-no-template-string-type)';
cookBookTag[78] =
  'Numeric value literals outside of integer range require long representation (arkts-use-long-for-large-numeric-literal)';
cookBookTag[79] = 'Type annotation in catch clause is not supported (arkts-no-types-in-catch)';
cookBookTag[80] = '"for .. in" is not supported (arkts-no-for-in)';
cookBookTag[81] = '';
cookBookTag[82] = '';
cookBookTag[83] = 'Mapped type expression is not supported (arkts-no-mapped-types)';
cookBookTag[84] = '"with" statement is not supported (arkts-no-with)';
cookBookTag[85] = '';
cookBookTag[86] = '';
cookBookTag[87] = '"throw" statements cannot accept values of arbitrary types (arkts-limited-throw)';
cookBookTag[88] = '';
cookBookTag[89] = '';
cookBookTag[90] = 'Function return type inference is limited (arkts-no-implicit-return-types)';
cookBookTag[91] = 'Destructuring parameter declarations are not supported (arkts-no-destruct-params)';
cookBookTag[92] = 'Nested functions are not supported (arkts-no-nested-funcs)';
cookBookTag[93] = 'Using "this" inside stand-alone functions is not supported (arkts-no-standalone-this)';
cookBookTag[94] = 'Generator functions are not supported (arkts-no-generators)';
cookBookTag[95] = '';
cookBookTag[96] = 'Type guarding is supported with "instanceof" and "as" (arkts-no-is)';
cookBookTag[97] = '';
cookBookTag[98] = '';
cookBookTag[99] =
  'It is possible to spread only arrays or classes derived from arrays into the rest parameter or array literals (arkts-no-spread)';
cookBookTag[100] = '';
cookBookTag[101] = '';
cookBookTag[102] = 'Interface can not extend interfaces with the same method (arkts-no-extend-same-prop)';
cookBookTag[103] = 'Declaration merging is not supported (arkts-no-decl-merging)';
cookBookTag[104] = 'Interfaces cannot extend classes (arkts-extends-only-class)';
cookBookTag[105] = '';
cookBookTag[106] = 'Constructor function type is not supported (arkts-no-ctor-signatures-funcs)';
cookBookTag[107] = '';
cookBookTag[108] = '';
cookBookTag[109] = '';
cookBookTag[110] = '';
cookBookTag[111] =
  'Enumeration members can be initialized only with compile time expressions of the same type (arkts-no-enum-mixed-types)';
cookBookTag[112] = '';
cookBookTag[113] = '"enum" declaration merging is not supported (arkts-no-enum-merging)';
cookBookTag[114] = 'Namespaces cannot be used as objects (arkts-no-ns-as-obj)';
cookBookTag[115] = '';
cookBookTag[116] =
  'Non-declaration statements in namespaces are not supported (single semicolons are considered as empty non-declaration statements) (arkts-no-ns-statements)';
cookBookTag[117] = '';
cookBookTag[118] = '';
cookBookTag[119] = '';
cookBookTag[120] = '';
cookBookTag[121] = '"require" and "import" assignment are not supported (arkts-no-require)';
cookBookTag[122] = '';
cookBookTag[123] = '';
cookBookTag[124] = '';
cookBookTag[125] = '';
cookBookTag[126] = '"export = ..." assignment is not supported (arkts-no-export-assignment)';
cookBookTag[127] = '';
cookBookTag[128] = 'Ambient module declaration is not supported (arkts-no-ambient-decls)';
cookBookTag[129] = 'Wildcards in module names are not supported (arkts-no-module-wildcards)';
cookBookTag[130] = 'Universal module definitions (UMD) are not supported (arkts-no-umd)';
cookBookTag[131] = '';
cookBookTag[132] = '"new.target" is not supported (arkts-no-new-target)';
cookBookTag[133] = '';
cookBookTag[134] = 'Definite assignment assertions are not supported (arkts-no-definite-assignment)';
cookBookTag[135] = '';
cookBookTag[136] = 'Prototype assignment is not supported (arkts-no-prototype-assignment)';
cookBookTag[137] = '"globalThis" is not supported (arkts-no-globalthis)';
cookBookTag[138] = 'Some of utility types are not supported (arkts-no-utility-types)';
cookBookTag[139] = 'Declaring properties on functions is not supported (arkts-no-func-props)';
cookBookTag[140] = '\'Function.bind\' is not supported (arkts-no-func-bind)';
cookBookTag[141] = 'Function("return this") is not supported (arkts-no-function-return-this)';
cookBookTag[142] = '"as const" assertions are not supported (arkts-no-as-const)';
cookBookTag[143] = 'Import assertions are not supported (arkts-no-import-assertions)';
cookBookTag[144] = 'Usage of standard library is restricted (arkts-limited-stdlib)';
cookBookTag[145] = 'Strict type checking is enforced (arkts-strict-typing)';
cookBookTag[146] = 'Switching off type checks with in-place comments is not allowed (arkts-strict-typing-required)';
cookBookTag[147] = 'No dependencies on TypeScript code are currently allowed (arkts-no-ts-deps)';
cookBookTag[148] =
  'Overriding with "readonly" field is not allowed when base field is not "readonly" (arkts-no-class-add-super-prop-with-readonly)';
cookBookTag[149] = 'Classes cannot be used as objects (arkts-no-classes-as-obj)';
cookBookTag[150] = '"import" statements after other statements are not allowed (arkts-no-misplaced-imports)';
cookBookTag[151] = 'Usage of \'ESValue\' type is restricted (arkts-limited-esobj)';
cookBookTag[152] = '\'Function.apply\', \'Function.call\' are not supported (arkts-no-func-apply-call)';
cookBookTag[153] = 'The inheritance for "Sendable" classes is limited (arkts-sendable-class-inheritance)';
cookBookTag[154] =
  'Properties in "Sendable" classes and interfaces must have a Sendable data type (arkts-sendable-prop-types)';
cookBookTag[155] =
  'Definite assignment assertion is not allowed in "Sendable" classes (arkts-sendable-definite-assignment)';
cookBookTag[156] =
  'Type arguments of generic "Sendable" type must be a "Sendable" data type (arkts-sendable-generic-types)';
cookBookTag[157] = 'Only imported variables can be captured by "Sendable" class (arkts-sendable-imported-variables)';
cookBookTag[158] = 'Only "@Sendable" decorator can be used on "Sendable" class (arkts-sendable-class-decorator)';
cookBookTag[159] =
  'Objects of "Sendable" type can not be initialized using object literal or array literal (arkts-sendable-obj-init)';
cookBookTag[160] =
  'Computed property names are not allowed in "Sendable" classes and interfaces (arkts-sendable-computed-prop-name)';
cookBookTag[161] = 'Casting "Non-sendable" data to "Sendable" type is not allowed (arkts-sendable-as-expr)';
cookBookTag[162] =
  'Importing a module for side-effects only is not supported in shared module (arkts-no-side-effects-imports)';
cookBookTag[163] = 'Only "Sendable" entities can be exported in shared module (arkts-shared-module-exports)';
cookBookTag[164] = '"export * from ..." is not allowed in shared module (arkts-shared-module-no-wildcard-export)';
cookBookTag[165] =
  'Only "Sendable" classes and "Sendable" interfaces are allowed for importing from ets into ts file (arkts-no-ts-import-ets)';
cookBookTag[166] =
  'In ts files, "Sendable" types cannot be used in implements and extends clauses (arkts-no-ts-sendable-type-inheritance)';
cookBookTag[167] =
  'In sdk ts files, "Sendable" class and "Sendable" interface can not be exported (arkts-no-dts-sendable-type-export)';
cookBookTag[168] = 'In ts files, entities from ets files can not be re-exported (arkts-no-ts-re-export-ets)';
cookBookTag[169] =
  'Namespace import is not allowed for importing from ets to ts file (arkts-no-namespace-import-in-ts-import-ets)';
cookBookTag[170] =
  'Side effect import is not allowed for importing from ets to ts file (artkts-no-side-effect-import-in-ts-import-ets)';
cookBookTag[171] = 'Field in sendable class must have type annotation (arkts-sendable-explicit-field-type)';
cookBookTag[172] =
  'Only imported variables can be captured by "Sendable" function (arkts-sendable-function-imported-variables)';
cookBookTag[173] = 'Only "@Sendable" decorator can be used on "Sendable" function (arkts-sendable-function-decorator)';
cookBookTag[174] =
  'Only "@Sendable" decorator can be used on "Sendable" typeAlias (arkts-sendable-typealias-decorator)';
cookBookTag[175] = 'Only "FunctionType" can declare "Sendable" typeAlias (arkts-sendable-typeAlias-declaration)';
cookBookTag[176] =
  'Only "Sendable" function or "Sendable" typeAlias object can be assigned to "Sendable" typeAlias (arkts-sendable-function-assignment)';
cookBookTag[177] =
  'When declaring "@Sendable" overloaded function, needs to add "@Sendable" decorator on each function (arkts-sendable-function-overload-decorator)';
cookBookTag[178] = 'The property of "Sendable" function is limited (arkts-sendable-function-property)';
cookBookTag[179] =
  'Casting "Non-sendable" function to "Sendable" typeAlias is not allowed (arkts-sendable-function-as-expr)';
cookBookTag[180] =
  'The "@Sendable" decorator can only be used on "class", "function" and "typeAlias" (arkts-sendable-decorator-limited)';
cookBookTag[181] = '';
cookBookTag[182] =
  'Sendable functions and sendable typealias are not available when compatibleSdkVersionStage is lowering than beta3 of API12 (arkts-sendable-beta-compatible)';
cookBookTag[183] = 'Object literal properties can only contain name-value pairs (arkts-obj-literal-props)';
cookBookTag[184] = 'Optional methods are not supported (arkts-optional-methods)';
cookBookTag[185] = 'syntax for import type is disabled (arkts-import-types)';
cookBookTag[186] = '"new" expression with dynamic constructor type is not supported (arkts-no-dynamic-ctor-call)';
cookBookTag[187] =
  'function "Math.pow()" behavior for ArkTS differs from Typescript version (arkts-math-pow-standard-diff)';
cookBookTag[188] =
  'In 1.1, the default type obtained for the abstract method without the annotation type is any. In 1.2, the default type for the abstract method without the annotation type is void. (arkts-distinct-abstract-method-default-return-type)';
cookBookTag[189] = 'Numeric semantics is different for integer values (arkts-numeric-semantic)';
cookBookTag[190] = 'Stricter assignments into variables of function type (arkts-incompatible-function-types)';
cookBookTag[191] = 'ASON is not supported. (arkts-no-need-stdlib-ason)';
cookBookTag[192] = 'Type "void" has no instances.(arkts-limited-void-type)';
cookBookTag[193] = '"void" operator is not supported (arkts-no-void-operator)';
cookBookTag[198] = 'Class TS overloading is not supported(arkts-no-ts-overload)';
cookBookTag[199] = 'Sendable containers are not supported (arkts-no-need-stdlib-sendable-containers)';
cookBookTag[202] = 'Literal types are restricted(arkts-limited-literal-types)';
cookBookTag[203] = 'exponent opartions "**" and "**=" are disabled (arkts-no-exponent-op)';
cookBookTag[206] = '"debugger" is not supported (arkts-no-debugger)';
cookBookTag[207] = 'Special arguments object inside functions are not supported (arkts-no-arguments-obj)';
cookBookTag[208] = 'Tagged templates are not supported (arkts-no-tagged-templates)';
cookBookTag[209] = 'The index expression must be of a numeric type (arkts-array-index-expr-type)';
cookBookTag[210] = 'The switch expression type must be of type number, string or enum (arkts-switch-expr)';
cookBookTag[211] = 'No two case constant expressions have identical values.(arkts-case-expr)';
cookBookTag[212] = 'The index expression must be zero or positive value.(arkts-array-index-negative)';
cookBookTag[213] = 'Class cannot have static codeblocks. (arkts-class-lazy-import)';
cookBookTag[214] = 'Objects have no constructor property in ArkTS1.2 (arkts-obj-no-constructor)';
cookBookTag[215] = 'Array bound not checked. (arkts-runtime-array-check)';
cookBookTag[216] =
  'The signature of a method in a class/interface cannot be different from the public interface in an object. (arkts-class-no-signature-distinct-with-object-public-api)';
cookBookTag[222] = 'Import for side-effect only is prohibited.(arkts-no-side-effect-import)';
cookBookTag[232] = 'Lazy import is not supported(arkts-no-lazy-import)';
cookBookTag[233] = 'Dynamic import is not supported(arkts-no-dynamic-import)';
cookBookTag[234] = 'Decorators are not supported(arkts-no-ts-decorators)';
cookBookTag[235] = 'Avoid using union types (arkts-common-union-member-access)';
cookBookTag[236] = 'Method can\'t override filed in interface implemented (arkts-no-method-overriding-field)';
cookBookTag[237] = 'Array and tuple are different type(arkts-no-tuples-arrays)';
cookBookTag[238] = 'The static property has no initializer (arkts-class-static-initialization)';
cookBookTag[239] = 'This keyword cannot be used as identifiers (arkts-invalid-identifier)';
cookBookTag[245] = 'JSON files cannot be imported (arkts-no-import-json-file)';
cookBookTag[249] =
  'The namespace imported by import * as cannot be used as a variable (arkts-no-import-namespace-with-star-as-var)';
cookBookTag[251] = '"!!" for bidirectional data binding is not supported (arkui-no-!!-bidirectional-data-binding)';
cookBookTag[252] = '"$$" for bidirectional data binding is not supported (arkui-no-$$-bidirectional-data-binding)';
cookBookTag[253] = '"${variable}" for decorator binding is not supported (arkui-link-decorator-passing)';
cookBookTag[254] = '"@Extend" decorator is not supported (arkui-no-extend-decorator)';
cookBookTag[255] = 'Extends or implements expression are not supported(arkts-no-extends-expression)';
cookBookTag[256] = '"@Styles" decorator is not supported (arkui-no-styles-decorator)';
cookBookTag[257] =
  '"@AnimatableExtend" decorator should be transformed to use receiver (arkui-animatableextend-use-receiver)';
cookBookTag[258] = 'Data observation needs to add "@Observed" (arkui-data-observation)';
cookBookTag[259] = 'The ArkUI interface should be imported before it is used (arkui-modular-interface)';
cookBookTag[260] = 'The "@Entry" annotation does not support dynamic parameters (arkui-entry-annotation-parameters)';
cookBookTag[261] =
  'The UIAbility of 1.2 needs to be listened by the new StaticAbilityLifecycleCallback. The original AbilityLifecycleCallback can only listen to the UIAbility of 1.1 (sdk-ability-lifecycle-monitor)';
cookBookTag[262] =
  'The "makeObserved" function cannot observe custom class (arkui-makeobserved-cannot-observe-custom-class)';
cookBookTag[263] =
  'The "@Provide" annotation does not support dynamic parameters (arkui-provide-annotation-parameters)';
cookBookTag[264] =
  'The field types of the subclass and parent class must be the same (arkts-class-same-type-prop-with-super)';
cookBookTag[265] = 'Direct inheritance of interop JS classes is not supported (arkts-interop-js2s-inherit-js-class)';
cookBookTag[266] = 'Direct usage of interop JS objects is not supported (arkts-interop-js2s-traverse-js-instance)';
cookBookTag[267] = 'Direct usage of interop JS functions is not supported (arkts-interop-js2s-js-call-static-function)';
cookBookTag[268] = 'Direct usage of interop JS objects is not supported (arkts-interop-js2s-condition-judgment)';
cookBookTag[269] =
  'Direct usage of interop JS functions is not supported (arkts-interop-js2s-js-expand-static-instance)';
cookBookTag[270] = 'ArkTS1.2 cannot catch a non Error instance thrown from JS code (arkts-interop-js2s-js-exception)';
cookBookTag[272] =
  'This API of process is obsolete in ArkTS 1.1. It\'s no longer supported in ArkTS 1.2 (arkts-concurrent-deprecated-apis)';
cookBookTag[273] =
  'Unsigned right shift on negative number yields different results in ArkTS versions. (arkts-distinct-unsigned-right-shift-negative-number)';
cookBookTag[274] =
  'The subclass constructor must call the parent class\'s parametered constructor (arkts-subclass-must-call-super-constructor-with-args)';
cookBookTag[275] =
  'The Custom component with custom layout capability needs to add the "@CustomLayout" decorator (arkui-custom-layout-need-add-decorator)';
cookBookTag[276] =
  'ArkTS 1.2 should implement all fields in the interface in the class (arkts-no-class-omit-interface-optional-prop)';
cookBookTag[277] = 'Creating local classes is not supported (arkts-no-local-class)';
cookBookTag[281] = '"@Prop" decorator is not supported (arkui-no-prop-decorator)';
cookBookTag[282] = '"@StorageProp" decorator is not supported (arkui-no-storageprop-decorator)';
cookBookTag[283] = '"@LocalStorageProp" decorator is not supported (arkui-no-localstorageprop-decorator)';
cookBookTag[284] = '"prop" function is not supported (arkui-no-prop-function)';
cookBookTag[285] = '"setAndProp" function is not supported (arkui-no-setandprop-function)';
cookBookTag[286] =
  'Parameters decorated with "@Prop" need to call the specific method when receiving data to ensure deep copy of the data (arkui-prop-need-call-method-for-deep-copy)';
cookBookTag[290] = 'Tuple type cannot be used in generic type parameters (arkts-not-support-tuple-generic-validation)';
cookBookTag[300] = 'The function type should be explicit (arkts-no-ts-like-function-call)';
cookBookTag[301] = 'Importing from "oh module" requires specifying full path (arkts-require-fullpath-name)';
cookBookTag[302] =
  'Class type is not compatible with "Object" parameter in interop call (arkts-interop-d2s-static-object-on-dynamic-instance)';
cookBookTag[303] =
  'Reflect API usage is not allowed in interop calls when an "Object" parameter receives a class instance (arkts-interop-d2s-static-reflect-on-dynamic-instance)';
cookBookTag[304] = 'Duplicate function name in namespace are not allowed (arkts-no-duplicate-function-name)';
cookBookTag[306] = 'Cannot access typescript types directly (arkts-interop-ts2s-static-access-ts-type)';
cookBookTag[307] = 'Trying to catch typescript errors is not permitted (arkts-interop-ts2s-ts-exception)';
cookBookTag[308] = 'Type "void" has no instances.(sdk-limited-void-type)';
cookBookTag[309] = 'API no longer supports optional methods (sdk-optional-methods)';
cookBookTag[310] =
  'Properties in "Sendable" classes and interfaces must have a Sendable data type (sdk-no-sendable-prop-types)';
cookBookTag[311] = 'Construct signatures are not supported in interfaces.(sdk-ctor-signatures-iface)';
cookBookTag[312] = 'Indexed access is not supported for fields (sdk-no-props-by-index)';
cookBookTag[313] = 'Constructor types are not supported - use lambda functions instead. (sdk-constructor-funcs)';
cookBookTag[314] =
  'Object property names must be valid identifiers.Single-quoted and hyphenated properties are not supported. (sdk-no-literal-as-property-name)';
cookBookTag[315] = 'API path have changed - please update your imports accordingly (sdk-no-decl-with-duplicate-name)';
cookBookTag[316] = 'Using typeof as a type is not allowed in this API (sdk-type-query)';
cookBookTag[317] = '"use shared" is not supported (arkts-limited-stdlib-no-use-shared)';
cookBookTag[318] = '"use concurrent" is not supported (arkts-limited-stdlib-no-use-concurrent)';
cookBookTag[319] =
  'Overridden method parameters and return types must respect type inheritance principles (arkts-method-inherit-rule)';
cookBookTag[321] = 'Import Concurrency is not required (arkts-limited-stdlib-no-import-concurrency)';
cookBookTag[322] = 'isConcurrent is not supported (arkts-limited-stdlib-no-support-isConcurrent)';
cookBookTag[323] = 'Direct export of interop JS objects is not supported (arkts-interop-js2s-export-js)';
cookBookTag[325] =
  'Default parameters must be placed after mandatory parameters (arkts-default-args-behind-required-args)';
cookBookTag[326] = 'It is not allowed to create object literal in interop calls (arkts-interop-s2d-object-literal)';
cookBookTag[327] =
  'Object literal not compatible with target union type. (arkts-interop-d2s-object-literal-no-ambiguity)';
cookBookTag[328] =
  'Object literal cannot be directly assigned to class with a constructor. (arkts-interop-d2s-object-literal-no-args-constructor)';
cookBookTag[329] = 'Enum cannot get member name by member value (arkts-enum-no-props-by-index)';
cookBookTag[330] = 'Importing directly from "JS" module is not supported (arkts-interop-js2s-import-js)';
cookBookTag[331] = 'ArkTS directly call JS functions or parameters is not supported (arkts-interop-js2s-call-js-func)';
cookBookTag[332] = 'Properties of interop objects can\'t be accessed directly (arkts-interop-js2s-access-js-prop)';
cookBookTag[333] = 'Casting interop JS objects to primitive types is not allowed (arkts-interop-js2s-convert-js-type)';
cookBookTag[334] = 'The "typeof" expression can\'t be used with interop JS objects (arkts-interop-js2s-typeof-js-type)';
cookBookTag[335] = 'Interop object does not have property num (arkts-interop-js2s-unary-op)';
cookBookTag[336] = 'Binary operations on js objects (arkts-interop-js2s-binary-op)';
cookBookTag[337] =
  'Importing data directly from the "JS" module for comparison is not supported (arkts-interop-js2s-compare-js-data)';
cookBookTag[338] =
  '"JS" objects can\'t be used directly as operands of the equality operators (arkts-interop-js2s-equality-judgment)';
cookBookTag[339] = 'Interop objects can\'t be indexed directly (arkts-interop-js2s-access-js-index)';
cookBookTag[340] = '"Await" operator can\'t be used with interop objects (arkts-interop-js2s-await-js-promise)';
cookBookTag[341] = 'ArkTS directly instantiated JS objects is not supported (arkts-interop-js2s-create-js-instance)';
cookBookTag[342] =
  'Calling methods of JS Object directly in interop is not allowed (arkts-interop-js2s-call-js-method)';
cookBookTag[343] =
  'Usage of "instanceof" operator is not allowed with interop objects (arkts-interop-js2s-instanceof-js-type)';
cookBookTag[344] = 'Interop objects can\'t be incremented or decremented (arkts-interop-js2s-self-addtion-reduction)';
cookBookTag[345] = 'Using thisArgs as a type is not allowed in this API (arkts-builtin-thisArgs)';
cookBookTag[346] = 'Using "Symbol.iterator" is not allowed in this API (arkts-builtin-symbol-iterator)';
cookBookTag[347] = 'Not support propertydescriptor (arkts-builtin-no-property-descriptor)';
cookBookTag[348] = 'API is not support ctor signature and func (arkts-builtin-cotr)';
cookBookTag[349] = 'SharedArrayBuffer is not supported (arkts-no-need-stdlib-sharedArrayBuffer)';
cookBookTag[350] =
  'The taskpool setCloneList interface is deleted from ArkTS1.2 (arkts-limited-stdlib-no-setCloneList)';
cookBookTag[351] =
  'The taskpool setTransferList interface is deleted from ArkTS1.2 (arkts-limited-stdlib-no-setTransferList)';
cookBookTag[352] =
  '1.2 Void cannot be combined. OnDestroy/onDisconnect (The return type of the method is now void | Promise) needs to be split into two interfaces. (sdk-ability-asynchronous-lifecycle)';
cookBookTag[355] = 'Usage of standard library is restricted(arkts-limited-stdlib-no-sendable-decorator)';
cookBookTag[356] = 'Usage of standard library is restricted(arkts-limited-stdlib-no-concurrent-decorator)';
cookBookTag[357] = 'Worker are not supported(arkts-no-need-stdlib-worker)';
cookBookTag[358] =
  'Using "Object.getOwnPropertyNames" is not allowed in this API (arkts-builtin-object-getOwnPropertyNames))';
cookBookTag[359] = '"@LocalBuilder" Decorator is not supported (arkui-no-localbuilder-decorator)';
cookBookTag[360] =
  '"Repeat" natively supports virtual scrolling capability in ArkTS1.2, so the default virtual scrolling should be disabled (arkui-repeat-disable-default-virtualscroll)';
cookBookTag[361] =
  'When using "WrappedBuilder", generics are required and must be declared as arrow function (arkui-wrappedbuilder-require-arrow-func-generic)';
cookBookTag[362] =
  'When using "wrapBuilder", generics must be declared as arrow function (arkui-wrapbuilder-require-arrow-func-generic)';
cookBookTag[363] = 'The generic of "BuilderNode" does not accept tuple (arkui-buildernode-generic-no-tuple)';
cookBookTag[364] =
  'The "update" interface of "BuilderNode" does not accept an object literal. Please replace it with an instance of the class specified in the generic when creating a new "BuilderNode", and ensure that the instance has the same field values as the literal (arkui-buildernode-update-no-literal)';
cookBookTag[365] = 'Property "nestingBuilderSupported" is not supported (arkui-buildernode-no-nestingbuildersupported)';
cookBookTag[366] = 'ESObject type cannot be used (arkts-no-esobject-support)';
cookBookTag[370] = 'Sparse array is not supported in ArkTS1.2 (arkts-no-sparse-array)';
cookBookTag[371] = 'Enum elements cannot be types in ArkTS1.2 (arkts-no-enum-prop-as-type)';
cookBookTag[372] = 'Smart type differences (arkts-no-ts-like-smart-type)';
cookBookTag[373] = 'Array type is immutable in ArkTS1.2 (arkts-array-type-immutable)';
cookBookTag[374] = 'Primitive types are normalized with their boxed type (arkts-primitive-type-normalization)';
cookBookTag[375] = 'TS catch type are not supported (arkts-no-ts-like-catch-type)';
cookBookTag[376] = 'Not supporting comparison between number type and bigint type (arkts-numeric-bigint-compare)';
cookBookTag[377] =
  'Non-decimal BigInt literals (0x/0o/0b) are not supported. Use decimal format instead (arkts-only-support-decimal-bigint-literal)';
cookBookTag[378] = 'Operator is not support (arkts-unsupport-operator)';
cookBookTag[381] =
  'The code block passed to stateStyles needs to be an arrow function (arkui-statestyles-block-need-arrow-func)';
cookBookTag[382] =
  'Promise<void>constructor only supports using resolve (undefined) (arkts-promise-with-void-type-need-undefined-as-resolve-arg)';
cookBookTag[391] =
  'The class of the second parameter passed to the "persistProp" method must be a primitive type or Date type, or implement the "toJson" and "fromJson" methods (arkui-persistent-prop-serialization)';
cookBookTag[392] =
  'The class of the "defaultValue" parameter in the literal passed to the "persistProps" method must be a primitive type or Date type, or implement the "toJson" and "fromJson" methods (arkui-persistent-props-serialization)';
cookBookTag[393] =
  'When calling the "globalConnect" and "connect" methods, the parameter list of the methods needs to include "toJson" and "fromJson" (arkui-persistencev2-connect-serialization)';
cookBookTag[399] = 'ArkUI deprecated api check (arkui-no-deprecated-api)';
cookBookTag[400] = 'ArkUI sdk common deprecated api check (arkui-sdk-common-deprecated-api)';
cookBookTag[401] = 'ArkUI sdk common whitelist api check (arkui-sdk-common-whitelist-api)';
cookBookTag[402] = 'ArkUI sdk common behavior change api check (arkui-sdk-common-behaviorchange-api)';
cookBookTag[403] = 'API is not support initial ctor signature (arkts-builtin-new-cotr)';
cookBookTag[404] =
  'Please init elements of array before read elements. If not, there will be a runtime error. We recommend you to use Array.create<T>(len, T) (arkts-builtin-uninitialized-element)';
cookBookTag[405] = 'API is not support use class in this API (arkts-builtin-final-class)';
cookBookTag[406] = 'Using narrowing of types is not allowed in this API (arkts-builtin-narrow-types)';
cookBookTag[407] = 'API has been disabled (arkts-builtin-disable-api)';
cookBookTag[408] = 'The property of IteratorResult is not supported (arkts-builtin-iterator-result-value)';

for (let i = 0; i <= cookBookTag.length; i++) {
  cookBookMsg[i] = '';
}
