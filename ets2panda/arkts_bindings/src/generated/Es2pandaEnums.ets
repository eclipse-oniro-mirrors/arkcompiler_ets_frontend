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

export enum Es2pandaContextState {
  ES2PANDA_STATE_NEW = 0,
  ES2PANDA_STATE_PARSED = 1,
  ES2PANDA_STATE_BOUND = 2,
  ES2PANDA_STATE_CHECKED = 3,
  ES2PANDA_STATE_LOWERED = 4,
  ES2PANDA_STATE_ASM_GENERATED = 5,
  ES2PANDA_STATE_BIN_GENERATED = 6,
  ES2PANDA_STATE_ERROR = 7
}