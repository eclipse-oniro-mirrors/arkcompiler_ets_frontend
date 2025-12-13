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

let maxConcurrentIO = 16;
let activeCount = 0;
const waitQueue: Array<() => void> = [];

export const FILES_PER_DIR_CONCURRENCY = 32;
export const DIRS_PER_DIR_CONCURRENCY = 16;
export const PROJECTS_PER_DIR_CONCURRENCY = 4;

export function setMaxIoConcurrency(max: number): void {
  if (Number.isInteger(max) && max > 0) {
    maxConcurrentIO = max;
  }
}

export async function runWithIOLimit<T>(fn: () => Promise<T>): Promise<T> {
  if (activeCount >= maxConcurrentIO) {
    await new Promise<void>((resolve) => {
      waitQueue.push(resolve);
    });
  }
  activeCount++;
  try {
    return await fn();
  } finally {
    activeCount--;
    const next = waitQueue.shift();
    if (next) {
      next();
    }
  }
}

export async function mapWithLimit<T, R>(
  items: T[],
  limit: number,
  mapper: (item: T, index: number) => Promise<R>
): Promise<R[]> {
  const results: R[] = [];
  let idx = 0;
  while (idx < items.length) {
    const batch = items.slice(idx, idx + limit);
    const batchResults = await Promise.all(
      batch.map((item, i) => {
        return mapper(item, idx + i);
      })
    );
    results.push(...batchResults);
    idx += limit;
  }
  return results;
}
