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

console.log(`${tag} resultcom:end`);
console.info(`${tag} resultcom:end`);
console.debug(`${tag} resultcom:end`);
console.warn(`${tag} resultcom:end`);
console.error(`${tag} resultcom:end`);
console.log(`resultcom:end`, tag);
console.log(tag, `resultcom:end`);
console.log(tag + `resultcom:end`);
console.log(`resultcom:end` + tag);
console.log('resultcom:end' + tag);
console.log("resultcom:end" + tag);

/* @@? 16:1 Error Semantic error ESE0127: No matching call signature for log("${tag} resultcom:end") */
/* @@? 16:16 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 17:1 Error Semantic error ESE0127: No matching call signature for info("${tag} resultcom:end") */
/* @@? 17:17 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 18:1 Error Semantic error ESE0127: No matching call signature for debug("${tag} resultcom:end") */
/* @@? 18:18 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 19:1 Error Semantic error ESE0127: No matching call signature for warn("${tag} resultcom:end") */
/* @@? 19:17 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 20:1 Error Semantic error ESE0127: No matching call signature for error("${tag} resultcom:end") */
/* @@? 20:18 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 21:1 Error Semantic error ESE0127: No matching call signature for log("resultcom:end", tag) */
/* @@? 21:30 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 22:1 Error Semantic error ESE0127: No matching call signature for log(tag, "resultcom:end") */
/* @@? 22:13 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 23:1 Error Semantic error ESE0127: No matching call signature for log("resultcom:endresultcom:end") */
/* @@? 23:13 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 24:1 Error Semantic error ESE0127: No matching call signature for log("resultcom:endresultcom:end") */
/* @@? 24:31 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 25:1 Error Semantic error ESE0127: No matching call signature for log("resultcom:endresultcom:end") */
/* @@? 25:31 Error Semantic error ESE0143: Unresolved reference tag */
/* @@? 26:1 Error Semantic error ESE0127: No matching call signature for log("resultcom:endresultcom:end") */
/* @@? 26:31 Error Semantic error ESE0143: Unresolved reference tag */
