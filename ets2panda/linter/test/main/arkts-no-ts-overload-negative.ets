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

import { Column, Row, ColumnOptions, RowOptions } from './sdk/column_row';

// Negative case 1: Column with explicit type assertion - no ambiguity error
Column({ space: 20 } as ColumnOptions)


// Negative case 2: Row with explicit type assertion - no ambiguity error
Row({ space: 20 } as RowOptions)


// Negative case 3: Column with no parameters - no ambiguity error
Column()


// Negative case 4: Row with no parameters - no ambiguity error
Row()


// Negative case 5: Using variable with explicit type instead of object literal - no ambiguity error
const columnOpts: ColumnOptions = { space: 20 };
Column(columnOpts) 


// Negative case 6: Using variable with explicit type for Row - no ambiguity error
const rowOpts: RowOptions = { space: 20 };
Row(rowOpts)


// Negative case 7: Column with explicit as ColumnOptions - no ambiguity error
Column({ space: 30 } as ColumnOptions)


// Negative case 8: Row with explicit as RowOptions - no ambiguity error
Row({ space: '40px' } as RowOptions)


// Negative case 9: Column with explicit as ColumnOptionsV2 - no ambiguity error
Column({ space: $r('app.string.space') } as ColumnOptionsV2)


// Negative case 10: Row with explicit as RowOptionsV2 - no ambiguity error
Row({ space: $r('app.string.space') } as RowOptionsV2)


// Negative case 11: Column with undefined parameter - no ambiguity error
Column(undefined)


// Negative case 12: Row with undefined parameter - no ambiguity error
Row(undefined)


// Negative case 13: Using typed variable with ColumnOptionsV2 - no ambiguity error
const columnOptsV2: ColumnOptionsV2 = { space: 50 };
Column(columnOptsV2)


// Negative case 14: Using typed variable with RowOptionsV2 - no ambiguity error
const rowOptsV2: RowOptionsV2 = { space: 60 };
Row(rowOptsV2) 

