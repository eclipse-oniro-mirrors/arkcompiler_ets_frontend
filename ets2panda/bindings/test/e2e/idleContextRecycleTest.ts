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

import fs from 'fs';
import { getLsp, getRealPath } from '../utils';
import { Lsp } from '../../src/lsp/lsp_helper';

describe('idleContextRecycleTest', () => {
  const moduleName = 'getReferencesAtPosition';

  // A full-suite jest worker can accumulate tens of GB of native RSS; past
  // maxProcessRssLimitMB every edit would trigger the synchronous emergency recycle
  // and shift the timer schedules these tests assert. The hard-limit branch is inert
  // here - the emergency behaviour is exercised through the mocked growth branch.
  const originalMaxProcessRssLimitMB = Lsp.maxProcessRssLimitMB;
  beforeAll(() => {
    Lsp.maxProcessRssLimitMB = 0;
  });
  afterAll(() => {
    Lsp.maxProcessRssLimitMB = originalMaxProcessRssLimitMB;
  });

  afterEach(() => {
    delete process.env.FORCE_LSP_IDLE_RECYCLE;
    jest.useRealTimers();
  });

  test('idle timer is scheduled and triggers context recycle on idle delay', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // 1. Modify document: should start idle timer (3s)
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // 2. Advance time past idleRecycleDelayMs: timer triggers recycle
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // 3. Verify recycled context is intact and queries still succeed with real references
      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
      expect(references!.some((ref) => ref.fileName.includes('getReferencesAtPosition2.ets') && ref.start === 635)).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('consecutive modifications reset the idle timer (debounce)', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      // First modification starts timer
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Advance by half of the idle delay
      jest.advanceTimersByTime(Math.floor(Lsp.idleRecycleDelayMs / 2));
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Second modification resets timer
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Advance to just before the rescheduled expiry -> timer still active
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs - 1000);
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Advance the remaining 1000ms -> idle delay reached, timer fires
      jest.advanceTimersByTime(1000);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('disposing Lsp helper cancels pending idle recycle timer', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    lsp.modifyFilesMap(testFile, { newDoc: source });
    expect(lsp.isIdleRecycleTimerActive()).toBe(true);

    lsp.dispose();
    expect(lsp.isIdleRecycleTimerActive()).toBe(false);

    // Advancing timers should not throw or trigger anything on disposed instance
    jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
    expect(lsp.isIdleRecycleTimerActive()).toBe(false);
  });

  test('active foreground query defers idle recycle timer', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Advance to 500ms before idle expiry
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs - 500);
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Foreground query occurs just before idle expiry
      const preRecycleRefs = lsp.getReferencesAtPosition(testFile, 635);
      expect(preRecycleRefs).toBeDefined();
      expect(preRecycleRefs!.length).toBeGreaterThan(0);

      // Advance 500ms to reach the original timer
      jest.advanceTimersByTime(500);
      // The recent query postpones the timer by the quiet window
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Advance the remaining quiet window; idle recycle completes and timer clears
      jest.advanceTimersByTime(Lsp.queryActivityQuietWindowMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // Verify query still works normally after deferred recycle
      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
      expect(references!.some((ref) => ref.fileName.includes('getReferencesAtPosition2.ets') && ref.start === 635)).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('continuous edits under normal memory do not trigger proactive reset, preserving warm cache', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      // Continuous edits without memory growth do not drop cache
      for (let i = 0; i < 30; i++) {
        lsp.modifyFilesMap(testFile, { newDoc: source });
      }
      expect(lsp.getConsecutiveEditsCount()).toBe(30);

      // Verify queries still work on the warm context
      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
      expect(references!.some((ref) => ref.fileName.includes('getReferencesAtPosition2.ets') && ref.start === 635)).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('emergency memory growth reaching threshold triggers proactive reset before edit', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      // Initial edit to establish baseline
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.getConsecutiveEditsCount()).toBe(1);

      // Simulate emergency memory growth exceeding threshold
      const baseline = process.memoryUsage().rss;
      const memSpy = jest.spyOn(process, 'memoryUsage').mockImplementation(() => ({
        rss: baseline + (Lsp.memoryGrowthThresholdMB + 10) * 1024 * 1024,
        heapTotal: 0,
        heapUsed: 0,
        external: 0,
        arrayBuffers: 0
      }));

      try {
        // Next edit detects dangerous memory growth -> triggers proactiveResetEHeap() before compiling
        lsp.modifyFilesMap(testFile, { newDoc: source });
        // Counter was reset to 0 by proactiveResetEHeap()
        expect(lsp.getConsecutiveEditsCount()).toBe(0);
      } finally {
        memSpy.mockRestore();
      }

      // Subsequent query works cleanly on newly compiled context
      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
      expect(references!.some((ref) => ref.fileName.includes('getReferencesAtPosition2.ets') && ref.start === 635)).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  // The case builds the AST cache (initAstCache), which resolves the fixture's dynamic
  // SDK declarations; environments whose ets/dynamic fixture cannot be parsed (gate
  // fixture builds) run with SKIP_UI_PLUGINS and skip it.
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)(
    'initAstCache records the warm baseline so the AST cache is not counted as growth',
    () => {
      jest.useFakeTimers();
      process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

      const astCacheModuleName = 'findRenameLocations';
      const lsp = getLsp(astCacheModuleName);
      const testFile = getRealPath(astCacheModuleName, 'findRenameLocations1.ets');
      const source = fs.readFileSync(testFile, 'utf8');

      try {
        // The AST-cache footprint appears after construction: every later RSS read is high.
        const baseline = process.memoryUsage().rss;
        const memSpy = jest.spyOn(process, 'memoryUsage').mockImplementation(() => ({
          rss: baseline + (Lsp.memoryGrowthThresholdMB + 10) * 1024 * 1024,
          heapTotal: 0,
          heapUsed: 0,
          external: 0,
          arrayBuffers: 0
        }));
        try {
          lsp.initAstCache();
        } finally {
          memSpy.mockRestore();
        }

        // The warm baseline absorbs the AST cache: the edit must not trigger an emergency
        // reset before it, so the consecutive edit counter keeps counting.
        lsp.modifyFilesMap(testFile, { newDoc: source });
        expect(lsp.getConsecutiveEditsCount()).toBe(1);
        expect(lsp.isIdleRecycleTimerActive()).toBe(true);

        const locations = lsp.findRenameLocations(testFile, 708);
        expect(locations).toBeDefined();
        expect(locations!.length).toBeGreaterThan(0);
      } finally {
        lsp.dispose();
        expect(lsp.isIdleRecycleTimerActive()).toBe(false);
      }
    }
  );

  test('idle recycle within the cooldown interval is deferred until the cooldown elapses', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      // 1. First idle recycle runs: the cooldown starts
      lsp.modifyFilesMap(testFile, { newDoc: source });
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // 2. Second idle window expires within the cooldown: no second recycle,
      //    the timer is rearmed for the rest of the cooldown instead
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // 3. Once the cooldown elapses the rearmed timer recycles
      jest.advanceTimersByTime(Lsp.minRecycleIntervalMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('growth-triggered emergency before edit respects the recycle cooldown', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      // 1. A completed idle recycle starts the cooldown and refreshes the baseline
      lsp.modifyFilesMap(testFile, { newDoc: source });
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
      const rssAfterRecycle = process.memoryUsage().rss;
      const baselineAfterRecycle = (lsp as unknown as { baselineRssMB: number }).baselineRssMB;
      expect(baselineAfterRecycle * 1024 * 1024).toBeGreaterThan(rssAfterRecycle - 64 * 1024 * 1024);

      // 2. Growth beyond the threshold within the cooldown must defer to the idle
      //    channel: the edit is not stalled by a synchronous destroy-and-rebuild.
      const memSpy = jest.spyOn(process, 'memoryUsage').mockImplementation(() => ({
        rss: rssAfterRecycle + (Lsp.memoryGrowthThresholdMB + 10) * 1024 * 1024,
        heapTotal: 0,
        heapUsed: 0,
        external: 0,
        arrayBuffers: 0
      }));
      try {
        lsp.modifyFilesMap(testFile, { newDoc: source });
        expect(lsp.getConsecutiveEditsCount()).toBe(1);
        expect(lsp.isIdleRecycleTimerActive()).toBe(true);
      } finally {
        memSpy.mockRestore();
      }

      // 3. The deferred recycle completes once the cooldown allows it
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);
      jest.advanceTimersByTime(Lsp.minRecycleIntervalMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('recycle followed by edit and query exercises incremental compilation correctly', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const initialSource = fs.readFileSync(testFile, 'utf8');

    try {
      // 1. Initial edit to populate context and start timer
      lsp.modifyFilesMap(testFile, { newDoc: initialSource });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // 2. Trigger idle recycle by expiring delay
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // 3. Verify recycled context has valid references
      const initialRefs = lsp.getReferencesAtPosition(testFile, 635);
      expect(initialRefs).toBeDefined();
      expect(initialRefs!.length).toBeGreaterThan(0);
      const initialCount = initialRefs!.length;

      // 4. Edit file content again after recycle - this exercises prepareFileCacheFromActiveContext
      const modifiedSource = initialSource + '\nlet extraVar = a;\n';
      lsp.modifyFilesMap(testFile, { newDoc: modifiedSource });

      // 5. Query references again: incremental compilation reflects the new reference
      const updatedRefs = lsp.getReferencesAtPosition(testFile, 635);
      expect(updatedRefs).toBeDefined();
      expect(updatedRefs!.length).toBe(initialCount + 1);
      expect(updatedRefs!.some((ref) => ref.fileName.includes('getReferencesAtPosition2.ets') && ref.start === 635)).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('consecutive query postponements are capped at maxIdleRecyclePostponeCount to prevent starvation', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Postponement can occur at most Lsp.maxIdleRecyclePostponeCount (3) times
      for (let i = 0; i < Lsp.maxIdleRecyclePostponeCount; i++) {
        const advanceBeforeExpiry = (i === 0 ? Lsp.idleRecycleDelayMs : Lsp.queryActivityQuietWindowMs) - 500;
        jest.advanceTimersByTime(advanceBeforeExpiry);
        // Active foreground query within quiet window
        lsp.getReferencesAtPosition(testFile, 635);
        // Advance 500ms to timer expiry
        jest.advanceTimersByTime(500);
        expect(lsp.isIdleRecycleTimerActive()).toBe(true);
        expect(lsp.getIdleRecyclePostponeCount()).toBe(i + 1);
      }

      // Next timer expiry: quiet window query happens, but postpone count is already at max (3)
      jest.advanceTimersByTime(Lsp.queryActivityQuietWindowMs - 500);
      lsp.getReferencesAtPosition(testFile, 635);
      jest.advanceTimersByTime(500);

      // Idle recycle is forced through, timer cleared and postpone count reset
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
      expect(lsp.getIdleRecyclePostponeCount()).toBe(0);

      // Context remains valid and usable
      const references = lsp.getReferencesAtPosition(testFile, 635);
      expect(references).toBeDefined();
      expect(references!.length).toBeGreaterThan(0);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('rename (getRenameInfo) succeeds after idle context recycle', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const renameModuleName = 'getRenameInfo';
    const lsp = getLsp(renameModuleName);
    const testFile = getRealPath(renameModuleName, 'getRenameInfo1.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Trigger idle recycle
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // Verify getRenameInfo succeeds on recycled context
      const res = lsp.getRenameInfo(testFile, 615);
      expect(res).toBeDefined();
      expect(res).toMatchObject({
        canRenameSuccess: true,
        displayName: 'aaa'
      });
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('hover (getQuickInfoAtPosition) succeeds after idle context recycle', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const quickInfoModuleName = 'getQuickInfoAtPosition';
    const lsp = getLsp(quickInfoModuleName);
    const testFile = getRealPath(quickInfoModuleName, 'getQuickInfoAtPosition1.ets');
    const source = fs.readFileSync(testFile, 'utf8');

    try {
      lsp.modifyFilesMap(testFile, { newDoc: source });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);

      // Trigger idle recycle
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // Verify getQuickInfoAtPosition succeeds on recycled context
      const res = lsp.getQuickInfoAtPosition(testFile, 639);
      expect(res).toBeDefined();
      expect(res?.kind).toBe('enum member');
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('cross-file references are preserved after idle context recycle', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile2 = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const testFile3 = getRealPath(moduleName, 'getReferencesAtPosition3.ets');
    const source2 = fs.readFileSync(testFile2, 'utf8');
    const source3 = fs.readFileSync(testFile3, 'utf8');

    try {
      // 1. Initial edit to both files to establish cross-file index
      lsp.modifyFilesMap(testFile2, { newDoc: source2 });
      lsp.modifyFilesMap(testFile3, { newDoc: source3 });

      // getReferencesAtPosition3.ets imports 'a' from getReferencesAtPosition2.ets
      const refsBefore = lsp.getReferencesAtPosition(testFile2, 635);
      expect(refsBefore).toBeDefined();
      expect(refsBefore!.some((ref) => ref.fileName.includes('getReferencesAtPosition3.ets'))).toBe(true);

      // 2. Switch back to testFile2 and trigger idle recycle
      lsp.modifyFilesMap(testFile2, { newDoc: source2 });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // 3. Query after recycle must still find cross-file references from getReferencesAtPosition3.ets
      const refsAfter = lsp.getReferencesAtPosition(testFile2, 635);
      expect(refsAfter).toBeDefined();
      expect(refsAfter!.length).toBe(refsBefore!.length);
      expect(refsAfter!.some((ref) => ref.fileName.includes('getReferencesAtPosition3.ets'))).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });

  test('compiledFileHashes is cleared on idle recycle so unsaved dependent files recompile properly', () => {
    jest.useFakeTimers();
    process.env.FORCE_LSP_IDLE_RECYCLE = 'true';

    const lsp = getLsp(moduleName);
    const testFile2 = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
    const testFile3 = getRealPath(moduleName, 'getReferencesAtPosition3.ets');
    const source2 = fs.readFileSync(testFile2, 'utf8');
    const source3 = fs.readFileSync(testFile3, 'utf8');

    try {
      // 1. Initial edit to both files to establish baseline
      lsp.modifyFilesMap(testFile2, { newDoc: source2 });
      lsp.modifyFilesMap(testFile3, { newDoc: source3 });
      const initialRefs = lsp.getReferencesAtPosition(testFile2, 635);
      expect(initialRefs).toBeDefined();
      const initialCount = initialRefs!.length;

      // 2. Switch to testFile2 and trigger idle recycle
      lsp.modifyFilesMap(testFile2, { newDoc: source2 });
      expect(lsp.isIdleRecycleTimerActive()).toBe(true);
      jest.advanceTimersByTime(Lsp.idleRecycleDelayMs);
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);

      // 3. Edit testFile3 in memory with an extra reference to 'a'
      const modifiedSource3 = source3 + '\nlet extra = a;\n';
      lsp.modifyFilesMap(testFile3, { newDoc: modifiedSource3 });

      // 4. Query references: incremental compilation should recompile testFile3 and reflect the extra reference
      const updatedRefs = lsp.getReferencesAtPosition(testFile2, 635);
      expect(updatedRefs).toBeDefined();
      expect(updatedRefs!.length).toBe(initialCount + 1);
      expect(updatedRefs!.some((ref) => ref.fileName.includes('getReferencesAtPosition3.ets'))).toBe(true);
    } finally {
      lsp.dispose();
      expect(lsp.isIdleRecycleTimerActive()).toBe(false);
    }
  });
});

