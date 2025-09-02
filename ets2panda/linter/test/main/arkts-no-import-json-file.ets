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

// Valid import (non-json)
import { config } from './config'; 

// Invalid import - should trigger arkts-no-import-json-file
import data from './data.json'; // error

// Another invalid case
import something from "../assets/data/data.json"; // error

// Still invalid even if aliased
import * as jsonData from "./mock.json"; // error

// Valid ArkTS-style config object (as alternative)
export const jsonLikeConfig = {
  port: 3000,
  secure: true,
  options: {
    retry: false
  }
};

// Usage of config object
const portNumber: number = jsonLikeConfig.port;
