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

import { KBoolean, KInt, KNativePointer } from '../common/InteropTypes';

export class Es2pandaNativeModule {
  _CreateMemberExpression(
    context: KNativePointer,
    object_arg: KNativePointer,
    property: KNativePointer,
    kind: KInt,
    computed: KBoolean,
    optional_arg: KBoolean
  ): KNativePointer {
    throw new Error('This methods was not overloaded by native module initialization');
  }
  _UpdateMemberExpression(
    context: KNativePointer,
    original: KNativePointer,
    object_arg: KNativePointer,
    property: KNativePointer,
    kind: KInt,
    computed: KBoolean,
    optional_arg: KBoolean
  ): KNativePointer {
    throw new Error('This methods was not overloaded by native module initialization');
  }
  _MemberExpressionObject(context: KNativePointer, receiver: KNativePointer): KNativePointer {
    throw new Error('This methods was not overloaded by native module initialization');
  }
  _MemberExpressionProperty(context: KNativePointer, receiver: KNativePointer): KNativePointer {
    throw new Error('This methods was not overloaded by native module initialization');
  }
  _MemberExpressionKindConst(context: KNativePointer, receiver: KNativePointer): KInt {
    throw new Error('This methods was not overloaded by native module initialization');
  }
}
