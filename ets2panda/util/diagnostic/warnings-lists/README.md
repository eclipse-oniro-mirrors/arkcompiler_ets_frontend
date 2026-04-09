# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

In our project there are currently soft lists and hard lists.
These mechanisms were introduced as a temporary solution for fixing existed code base.
Both of them are used, and developers can chose which one is better to use in their case.

All file-paths passed to es2anda are checked via regular expressions.
If they belong to a soft or a hard list or both of them
There are some warnings that should be CTE according to our specification. 
Lets name them "bad warnings" further.

Hard lists - if a path of a current file matches the particular hard list (every "bad warning" has its own hard list), all "bad warnings" of this file become CTE.

Soft list - if a path of a current file does NOT match the particular soft list (every "bad warning" has its own soft list as well), all "bad warnings" of this file become CTE. If a file is out of a soft list, all "bad warnings" become CTE.

Note: if a file matches both hard and soft lists, the hard lists will have higher priority, so all "bad warnings" of this file become CTE.

The soft list is the list that shows how many repositories/subdirectories/files are left to fix, and switch "bad warning" to a true CTE.

How to use soft list/hard list:
1. Write soft list / hard list in softlist.yaml / hardlist.yaml with some corresponding name and regular expressions for path match, for example: name: "allow_lib" / "ban lib" paths: ".test/lib/.", or add new paths in already existing one. 
2. Add this soft list / hard list name to array in softlist / hardlist field in warning.yaml, or if there is no such field ("softlist" / "hardlist") add it (softlist: [] / hardlist: []) and then add soft list / hard list name to it. You can also find examples in ets_frontend/ets2panda/util/diagnostic/warning.yaml, hardlist.yaml and softlist.yaml

All "bad warnings" are marked with `strict: true` flag in ets_frontend/ets2panda/util/diagnostic/warning.yaml

Some recommendations of which list is better to use:
- If you have many files that you want to suppress - use soft list
- If your repository/subdirectory is already in soft list, and you want to partly fix several files from it,
it is better to add them into hard list, and do not modify whitelist. Remember that priority is higher for 
hard lists if there is an intersection between two lists for the same warning.
