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

import * as named from './namespace_export1.ets'
import * as production from './namespace_export2.ets'

arktest.assertEQ( /* @@ label1 */production, null, 'default specified via identifier');
arktest.assertEQ( /* @@ label2 */named, null, 'default specified via identifier');

/* @@@ label1 Error TypeError: Namespace 'production' cannot be used as object */
/* @@@ label2 Error TypeError: Namespace 'named' cannot be used as object */