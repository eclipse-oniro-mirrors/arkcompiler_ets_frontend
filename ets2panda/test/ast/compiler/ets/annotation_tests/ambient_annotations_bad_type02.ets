/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

import {ClassAuthor, Color} from "./annotation_src"

@interface ClassAuthor {
}

/* @@? 18:2 Error TypeError: Field 'mutiArray' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'color' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'testBools' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'reviewers' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'favorColor' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'testBool' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'reviewersAge' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'authorAge' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */
/* @@? 18:2 Error TypeError: Field 'authorName' in annotation 'ClassAuthor' is declared in the ambient declaration but missing in the implementation.  */