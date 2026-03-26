/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export class DiagnosticCase {
  text: string;

  constructor(text: string) {
    this.text = text;
  }
}

export function BuildDiagnosticCases(): DiagnosticCase[] {
  const valid = new DiagnosticCase("ok");
  const alsoValid = new DiagnosticCase("stable");
  return [valid, alsoValid];
}

// Keep a couple of tricky patterns for parser/semantic diagnostics benches.
function ShadowingCase(value: number): number {
  const local = value + 1;
  if (local > 0) {
    const value = local * 2;
    return value;
  }
  return local;
}

export function RunDiagnosticScenario(seed: number): number {
  return ShadowingCase(seed);
}
