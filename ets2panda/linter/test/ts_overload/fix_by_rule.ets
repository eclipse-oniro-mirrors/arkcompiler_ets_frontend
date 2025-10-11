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

import systemShare from './sdk/@ohos.accessibility'
import sharesystem from './sdk/@ohos.data.sendablePreferences'
import cooperate from './sdk/@ohos.cooperate.d.ts'

//TS File
let a = new systemShare.A()
a.off('test1', 234) // no fix, not exist in uniontype
a.off('remove', 234) // will be fixed
a.off('change', 234) // will be fixed
a.test1('test2', 234) //no fix, not target method
a.on(333, 234) // no fix, passing number
a.once('sds', 234) // no fix, passing string instead expected type
a.off(333, 234) // no fix
a.code(true, 44) // no fix

systemShare.off('waterMarkFlagChange', 'cihat') // will fixed
systemShare.on('knockShare') // will fixed
systemShare.once('knockShare') //no fix, not have type parameter


//ETS file
let test2 = new sharesystem.B();

test2.fail('shareCompleted', 234) // no fix
test2.once('add', 234) // fix
test2.on('slice', 234) // fix
test2.off('test3', 234) // no fix, passing string, not type
test2.off('obj', 234) // fix
test2.off(333, 234) // no fix
test2.code(true, 44) // no fix

sharesystem.off('waterMarkFlagChange', 'cihat') // fix
sharesystem.on('knockShare') // fix
sharesystem.on('change') // fix
sharesystem.on('test4') // no fix
sharesystem.once('knockShare') //no fix


//should be ignored
cooperate.on('cooperate', () => {})
cooperate.off('cooperate', () => {})