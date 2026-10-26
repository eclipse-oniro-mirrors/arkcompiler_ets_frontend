/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

@Entry
@Component
struct Index {
    @State dataSource: Array<number> = [1.0, 2.0, 3.0]

    @Builder
    EndSwipeActionBuilder() {}

    build() {
        Column() {
            List({ space: 8.0 }) {
                ForEach(this.dataSource, (item: number) => {
                    ListItem() {
                        Text(`item-${item}`)
                    }.width(200).height(100)
                    .swipeAction({
                        end: this.EndSwipeActionBuilder()
                    })
                })
            }
        }
    }
}
