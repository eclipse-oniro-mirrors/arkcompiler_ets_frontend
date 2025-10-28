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

import type { Autofix } from '../../autofixes/Autofixer';

export enum GenericPassingCheck {
  NON_RESOLVED,
  FIXED
}

export interface GenericCheckFixed {
  kind: GenericPassingCheck.FIXED;
  autofix: Autofix[] | undefined;
}

export interface GenericCheckNonResolved {
  kind: GenericPassingCheck.NON_RESOLVED;
}

export type GenericPassingType = GenericCheckFixed | GenericCheckNonResolved;

export function GENERIC_PASSING_NON_RESOLVED(): GenericPassingType {
  return {
    kind: GenericPassingCheck.NON_RESOLVED
  };
}

export function GENERIC_PASSING_FIXED(autofix: Autofix[] | undefined): GenericPassingType {
  return {
    kind: GenericPassingCheck.FIXED,
    autofix
  };
}
