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

export function getMonthDate(specifiedMonth: number, specifiedYear: number): number[]{
  let currentFirstWeekDay: number = 0;
  let currentLastWeekDay: number = 0;
  let currentAllDay: number[] = [];
  let totalDays: int = new Date(specifiedYear, specifiedMonth, 0).getDate();
  currentFirstWeekDay = new Date(specifiedYear, specifiedMonth - 1, 1).getDay();
  currentLastWeekDay = new Date(specifiedYear, specifiedMonth - 1, totalDays).getDay();

  for (let item: int = 0; item < currentFirstWeekDay; item++) {
    currentAllDay[item] = 0;
  }
  for (let item: int = 1; item <= totalDays; item++) {
    currentAllDay.push(item);
  }
  for (let item: int = 0; item < 6 - currentLastWeekDay; item++) {
    currentAllDay.push(0);
  }
  return currentAllDay;
}
