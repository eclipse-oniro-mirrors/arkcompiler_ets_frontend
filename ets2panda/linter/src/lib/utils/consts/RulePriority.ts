/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Rules in priority order: highest priority (based on frequency) first
const RULES_BY_PRIORITY: readonly string[] = [
  'arkui-modular-interface',
  'arkts-require-fullpath-name',
  'arkts-numeric-semantic',
  'arkts-runtime-array-check',
  'arkts-no-lazy-import',
  'arkui-deprecated-interface',
  'arkts-no-ts-like-smart-type',
  'arkts-no-props-by-index',
  'arkts-no-ts-like-function-call',
  'arkts-array-index-expr-type',
  'arkts-no-inferred-generic-params',
  'arkts-instance-method-bind-this',
  'arkts-limited-void-type',
  'arkui-no-prop-decorator',
  'arkui-data-observation',
  'arkts-no-classes-as-obj',
  'arkts-method-inherit-rule',
  'arkts-incompatible-function-types',
  'arkts-array-type-immutable',
  'arkts-no-structural-typing',
  'arkui-no-storageprop-decorator',
  'arkts-no-ts-decorators',
  'arkts-no-sparse-array',
  'arkui-no-styles-decorator',
  'arkts-no-globalthis',
  'arkts-class-static-initialization',
  'arkui-no-extend-decorator',
  'arkts-common-union-member-access',
  'arkts-no-ts-like-as',
  'arkui-no-update-in-build',
  'arkts-identifiers-as-prop-names',
  'arkts-builtin-cotr',
  'arkts-subclass-must-call-super-constructor-with-args',
  'arkts-invalid-identifier',
  'arkts-obj-no-constructor',
  'arkts-no-regexp-literals',
  'arkts-no-enum-no-props-by-index',
  'arkui-provide-annotation-parameters',
  'arkts-no-tuples-arrays',
  'arkts-obj-literal-props',
  'sdk-ctor-signatures-iface',
  'arkts-limited-literal-types',
  'arkui-entry-annotation-parameters',
  'sdk-limited-void-type',
  'arkui-custombuilder-passing',
  'arkts-no-ts-like-catch-type',
  'arkts-no-enum-mixed-types',
  'arkts-no-method-overriding-field',
  'arkui-statestyles-block-need-arrow-func',
  'arkts-no-func-bind',
  'arkts-limited-stdlib-no-import-concurrency',
  'arkui-no-$$-bidirectional-data-binding',
  'arkts-no-need-stdlib-sendable-containers',
  'arkts-no-ts-overload',
  'arkui-no-localstorageprop-decorator',
  'arkts-default-args-behind-required-args',
  'arkts-no-enum-prop-as-type',
  'arkui-repeat-disable-default-virtualscroll',
  'arkts-unsupport-operator',
  'arkts-no-need-stdlib-worker',
  'arkts-primitive-type-normalization',
  'arkts-builtin-no-property-descriptor',
  'arkui-animatableextend-use-receiver',
  'arkts-limited-stdlib-no-sendable-decorator',
  'arkts-obj-literal-generate-class-instance',
  'arkui-no-!!-bidirectional-data-binding',
  'arkts-limited-stdlib',
  'arkts-builtin-object-getOwnPropertyNames',
  'arkui-no-localbuilder-decorator',
  'arkts-no-void-operator',
  'arkts-no-need-stdlib-sharedArrayBuffer',
  'arkts-case-expr',
  'arkts-no-ctor-prop-decls',
  'arkui-custom-layout-need-add-decorator',
  'arkts-no-arguments-obj',
  'arkts-no-side-effect-import',
  'arkts-limited-stdlib-no-use-shared',
  'arkui-no-prop-function',
  'arkts-no-alias-on-const'
];

// Build map from rule name to priority for O(1) lookup
const RULE_PRIORITY_MAP: Readonly<Record<string, number>> = Object.fromEntries(
  RULES_BY_PRIORITY.map((rule, index) => { return [rule, RULES_BY_PRIORITY.length - index]; })
);

export function getRulePriority(rule: string): number {
  // Undefined rules have priority 0 (lowest)
  return RULE_PRIORITY_MAP[rule] ?? 0;
}
