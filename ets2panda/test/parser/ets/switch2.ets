/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

// see 6.12 - value switch
function main(): void {
	let value: byte = 10;
	let result: byte = 0;
	switch (value) {
		case 0:
		case 1:
			result = 1;
			break;
		case 2:
			result = 2;
			break;
		case 127:
			break;
		case 128:
			break;
		case -129:
			break;
		default:
			result = -1;
	}
}
