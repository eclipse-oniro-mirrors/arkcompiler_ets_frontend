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
 
import { Cjs } from '../main/js_lib';
import { fjs } from '../main/js_lib';
import { CPreview,bar,foo } from "./jsfiles/preview_import_js";
import myAaa from "./interop_import_js_js";
import myAaa,{ClassA,Dog} from "./interop_import_js_js";
import * as tjs from "./interop_import_js_js";
import { Wiki, Dog as Doge } from './interop_import_js_js'