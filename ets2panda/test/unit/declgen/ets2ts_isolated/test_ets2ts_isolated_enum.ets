/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
export enum Direction {
  Up,
  Down,
  Left,
  Right
}

export enum Message {
  Success = "SUCCESS",
  Failure = "FAILURE",
  Pending = "PENDING"
}

export enum NumbericEnumWithOps {
  One = 1,
  Two = 1+1,
  Three = 1+2,
  Four = 2*2,
  Five = 10/5+(1*2)+1,
  Six,
}
