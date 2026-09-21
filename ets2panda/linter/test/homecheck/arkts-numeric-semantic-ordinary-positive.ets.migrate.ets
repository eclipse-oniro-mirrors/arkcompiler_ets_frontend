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

const ORDINARY_POSITIVE_VALUES: string[] = ['zero', 'one', 'two'];

function decimalNumberCalculation(): void {
  let decimalValue: number = 1.5;
  let quotient: number = 4.0 / 2.0;
  console.log(decimalValue + quotient);
}

function integerLiteralIndexWithoutNumberAnnotation(): void {
  console.log(ORDINARY_POSITIVE_VALUES[1]);
}

function explicitIntLocalIndex(): void {
  let index: int = 1;
  console.log(ORDINARY_POSITIVE_VALUES[index]);
}

function explicitIntLoopIndex(): void {
  for (let index: int = 0; index < 2; index++) {
    console.log(ORDINARY_POSITIVE_VALUES[index]);
  }
}

function numberParameterUsedAsNumber(value: number): void {
  console.log(value / 2.0);
}

class ExplicitIntIndexHolder {
  private index: int = 1;
  private static staticIndex: int = 2;

  readInstance(): string {
    return ORDINARY_POSITIVE_VALUES[this.index];
  }

  static readStatic(): string {
    return ORDINARY_POSITIVE_VALUES[ExplicitIntIndexHolder.staticIndex];
  }
}
