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

import { LANGUAGE_VERSION } from '../pre_define';
import { isFirstLineUseStatic } from '../util/utils';

/**
 * Adapter for the build-environment facts needed while resolving interop
 * configurations.
 *
 * Custom hosts are merged over the default host the way a customized
 * `ts.CompilerHost` is: every provided method replaces its default, and
 * unprovided methods fall back to the default implementation.
 */
export interface InteropConfigHost {
  /**
   * Reads the file at `filePath` and returns the language its source code
   * declares. Receives an absolute path; implementations normalize it as needed.
   */
  getLanguageFromSourceCode(filePath: string): LANGUAGE_VERSION;
}

/** Creates the default host that classifies sources via build_system's `'use static'` first-line check. */
export function createDefaultInteropConfigHost(): InteropConfigHost {
  return {
    getLanguageFromSourceCode(filePath: string): LANGUAGE_VERSION {
      return isFirstLineUseStatic(filePath) ? LANGUAGE_VERSION.ARKTS_1_2 : LANGUAGE_VERSION.ARKTS_1_1;
    },
  };
}

/**
 * Creates a host where each provided custom method overrides its default,
 * mirroring how `ts.CompilerHost` instances are customized. Explicitly
 * undefined methods keep the default instead of overriding it.
 *
 * New host methods must be wired into this merge explicitly.
 */
export function createInteropConfigHost(custom: Partial<InteropConfigHost>): InteropConfigHost {
  const host = createDefaultInteropConfigHost();
  if (custom.getLanguageFromSourceCode !== undefined) {
    host.getLanguageFromSourceCode = custom.getLanguageFromSourceCode;
  }
  return host;
}
