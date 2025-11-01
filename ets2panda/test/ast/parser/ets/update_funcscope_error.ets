/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

type UpdateFunc = (u: Record<string, Object>) => Record<string, Object>;
type ReducerFunc = (key: string) => Record<string, Object>;

export const updateIfChanged = (t: Record<string, Object>) => {
  const reducerFunc: ReducerFunc = (key) => {
    const value = u[key];
    const reduceResult = reduce(
      value as Record<string, Object>,
      (v: Record<string, Object>) => {
        const baseU = u;
        const updatedU: Record<string, Object> = {
          ...baseU,
          [key]: v
        };
        return update(updatedU);
      }
    );
    return reduceResult;
  };
};

/* @@? 21:19 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 21:19 Error Semantic error ESE0143: Unresolved reference u */
/* @@? 24:7 Error Semantic error ESE0046: Type '((p1: Record<String,Object>) => Boolean)' is not compatible with type '((p1: Boolean, p2: Boolean, p3: Int, p4: FixedArray<Boolean>) => Boolean)' at index 2 */
/* @@? 27:11 Error Semantic error ESE0393: Invalid record property */
/* @@? 28:11 Error Syntax error ESY0016: Unexpected token. */
/* @@? 28:12 Error Syntax error ESY0228: Unexpected token, expected '('. */
/* @@? 28:12 Error Semantic error ESE0046: Type '*ERROR_TYPE*' is not compatible with type 'String' at index 1 */
/* @@? 28:15 Error Syntax error ESY0016: Unexpected token. */
/* @@? 28:15 Error Syntax error ESY0230: Expected '{', got ']'. */
/* @@? 28:15 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 28:15 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 28:15 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 28:16 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 28:18 Error Syntax error ESY0227: Unexpected token 'v'. */
/* @@? 29:10 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 30:16 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 30:16 Error Semantic error ESE0143: Unresolved reference update */
/* @@? 32:5 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 33:12 Error Semantic error ESE0143: Unresolved reference reduceResult */
/* @@? 35:1 Error Syntax error ESY0227: Unexpected token '}'. */
