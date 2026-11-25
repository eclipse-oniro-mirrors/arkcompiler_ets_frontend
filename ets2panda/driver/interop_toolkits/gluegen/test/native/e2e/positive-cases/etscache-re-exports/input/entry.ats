/**
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

// Re-exports from a *virtual-path* static module -- the binary-ABC-dependency scenario. The
// 'pkg/src/foo' import resolves (via the `dependencies` entry of the generated default
// arktsconfig) to the virtual module path "pkg/src/foo" rather than an on-disk file, and the
// module's declarations come from the etscache file under
// prefabs/etscache-modules/pkg/src/foo.etscache (copied next to the generated arktsconfig at
// configure time), which ImportPathManager loads as an ETSCACHE_DECL program. The re-exports
// must expand from that program's symbols instead of being dropped with an
// UNRESOLVED_REEXPORT_SOURCE warning.
export { StaticClassA } from 'pkg/src/foo';
export { staticFunctionA as AliasedFunction } from 'pkg/src/foo';
export * from 'pkg/src/foo';
