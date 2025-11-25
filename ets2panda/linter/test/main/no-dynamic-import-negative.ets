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

import('./file').then(m => { // Dynamic import is not supported
  console.log(m.Data);
});

async function loadData() {
  const mod = await import('./data'); //Dynamic import is not supported
  console.log(mod);
}

document.getElementById("btn")?.addEventListener("click", async () => {
  const module = await import('./utils'); // Dynamic import inside callback is not supported
});

function getModule() {
  return import('./heavy'); // Returning a dynamic import is not supported
}

const loader = import('./foo/bar'); // Assigning a dynamic import to variable is not supported

if (true) {
  import('./conditional').then(m => console.log(m)); // Dynamic import inside condition is not supported
}

const result = (await import('./calc')).default(2, 3); // Dynamic import used inside expression is not supported
