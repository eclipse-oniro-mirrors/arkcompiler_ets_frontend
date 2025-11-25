/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

import eslintPluginTS from '@typescript-eslint/eslint-plugin';
import tsParser from '@typescript-eslint/parser';

export default [
  {
    ignores: [
      'dist/**/*',
      'build/**/*',
      'docs/**/*',
      'node_modules/**/*',
      'test/**/*',
      '**/**.json',
      '**/**.js'
    ]
  },
  {
    files: ['**/*.ts'],
    plugins: {
      '@typescript-eslint': eslintPluginTS,
    },
    languageOptions: {
      parser: tsParser,
        parserOptions: {
          project: './tsconfig.json',
          tsconfigRootDir: import.meta.dirname
        }
    },
    rules: {
      "quotes": ["error", "single", { "avoidEscape": true, "allowTemplateLiterals": false }],
      "eqeqeq": ["error", "always"],
      "curly": ["error", "all"],
      "brace-style": ["error", "1tbs", { "allowSingleLine": false }],
      "max-depth": ["error", { "max": 4 }],
      "no-extra-bind": "error",
      "no-lonely-if": "error",
      "no-unneeded-ternary": "error",
      "no-useless-return": "error",
      "no-var": "error",
      "spaced-comment": ["error", "always"],
      "one-var": ["error", "never"],
      "@typescript-eslint/explicit-function-return-type": "error",
      "@typescript-eslint/adjacent-overload-signatures": "error",
      "@typescript-eslint/no-confusing-non-null-assertion": "error",
      "@typescript-eslint/no-confusing-void-expression": "error",
      "@typescript-eslint/no-meaningless-void-operator": "error",
      "@typescript-eslint/prefer-as-const": "error",
      "@typescript-eslint/consistent-type-exports": "error",
      "@typescript-eslint/await-thenable": "error",
      "@typescript-eslint/no-dynamic-delete": "warn",
      "@typescript-eslint/no-this-alias": "error",
      "@typescript-eslint/no-unsafe-member-access": "warn",
      "@typescript-eslint/no-unsafe-call": "warn",
      "@typescript-eslint/no-unsafe-argument": "warn",
      "@typescript-eslint/no-unsafe-return": "warn",
      "@typescript-eslint/no-explicit-any": "warn",
      "no-unsafe-finally": "error",
      "@typescript-eslint/no-unnecessary-condition": "off",
      "@typescript-eslint/naming-convention": [
        "off",
        {
          "selector": "default",
          "format": ["camelCase"]
        },
        {
          "selector": "enumMember",
          "format": ["UPPER_CASE"]
        },
        {
          "selector": "variable",
          "format": ["camelCase", "UPPER_CASE"]
        },
        {
          "selector": "typeLike",
          "format": ["PascalCase"]
        },
        {
          "selector": "memberLike",
          "format": ["camelCase"]
        }
      ]
    }
  }
]