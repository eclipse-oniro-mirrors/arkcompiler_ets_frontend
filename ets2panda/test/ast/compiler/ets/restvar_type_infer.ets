/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable low or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

function main() {
    let r1 = ((...args) =>{ return args.length; })(1,2,3);
    let r2 = ((a:number, ...args)=> { return args.length; })(1,2,3,4,5)
}


/* @@? 17:14 Error Semantic error ESE0078: Cannot use type 'void' as value. */
/* @@? 17:16 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 17:16 Error Semantic error ESE0132: The type of parameter 'args' cannot be inferred */
/* @@? 18:14 Error Semantic error ESE0078: Cannot use type 'void' as value. */
/* @@? 18:26 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 18:26 Error Semantic error ESE0132: The type of parameter 'args' cannot be inferred */