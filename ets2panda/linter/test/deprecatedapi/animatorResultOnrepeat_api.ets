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
import  Animator as animator, AnimatorResult  from './sdk/api/@ohos.animator';

let AnimatorResult: AnimatorResult | undefined = animator.create(options)

AnimatorResult.onrepeat = () => {       //error
  console.info("onrepeat callback")
}

AnimatorResult.oncancel = () => {       //error
  console.info("oncancel callback")
}

AnimatorResult.onfinish = () => {       //error
  console.info("onfinish callback")
}

AnimatorResult.onframe = (value) => {   //error
  console.info("onframe callback")
}