# Logic
- Run es2panda for benchmark files from this directory
- Dump perfmetrics to `<work_dir>/test-current-perf.txt` for current es2panda
- Dump perfmetrics to `<work_dir>/test-pre_merge-perf.txt` for another (pre-merge) es2panda
- Dump comparison report to `<work_dir>/test-report.txt` in format like `time=-90.00ms (-1.9%)`

### Static mode
If `actual_perf > max_perf * (1 + static_regression)` - an error occurs. Example:
```
[PERF REGRESSION] Failed for bench_1-current-perf.txt: Memory exceeded threshold.
  Limit: 5.0%, Actual: +406.25%
  Base: 32.00MB, New: 162.00MB
  Threshold: < 33.60MB
```

If `actual_perf < max_perf * (1 + 3 * static_regression)` - an error occurs. Example:
```
[UPDATE REQUIRED] Very good perf for bench_1-current-perf.txt: Please update *-max.txt.
  Hint: use flag '--dump-perf-metrics' and Release build of es2panda.
```

### Dynamic mode
If `actual_perf > pre_merge_perf * dynamic_regression` - an error occurs (the same as in static mode).

### Errors reporting
Errors are printed to the console and also to `<work_dir>/error_log.txt`.

# Arguments
- `--mode` - 'static' to compare with `*-max.txt` files. 'dynamic' to compare with pre-merge es2panda.
- `--es2panda` - Path to current es2panda (aka <build>/bin/es2panda)
- `--es2panda-pre-merge` - Path to pre-merge es2panda (aka <pre_merge_build>/bin/es2panda)
- `--test-dir` - Path to test directory with test files
- `--work-dir` - Path to the working temp folder with gen, intermediate and report folders
- `--dynamic-regression` - Acceptable regression compared to the another (pre-merge) es2panda
- `--static-regression` - Acceptable regression compared to static vales from `*-max.txt` files
- `--runs` - The number of runs to average
- `--werror` - Warnings as errors

# Max values
Each file have companion: for `test.ets` companion is `test-max.txt`. This file contains max values for metrics.

# Local reproduction
```bash
# static mode
python3 <ets_frontend>/ets2panda/test/benchmarks/runner/runner.py --mode=static --es2panda=<build>/bin/es2panda --work-dir=<build>/e2p_benchmarks --test-dir=<ets_frontend>/ets2panda/test/benchmarks

# dynamic mode
python3 <ets_frontend>/ets2panda/test/benchmarks/runner/runner.py --mode=dynamic --es2panda=<build>/bin/es2panda --work-dir=<build>/e2p_benchmarks --test-dir=<ets_frontend>/ets2panda/test/benchmarks --es2panda-pre-merge=<pre_merge_build>/bin/es2panda
```
See `--help` if needed.

# CI
You can download artifacts for this job with perf stat.

# Artifacts example

test-perf.txt
```
================ es2panda perf metrics (Averaged over 3 runs) ================
:@phases                                        :  time=891.00ms      mem=140.00MB
:@phases/ConstantExpressionLowering             :  time=233.00ms      mem=0.26MB
:@phases/TopLevelStatements                     :  time=193.00ms      mem=79.00MB
:@phases/ResolveIdentifiers                     :  time=83.40ms       mem=6.00MB
:@phases/CheckerPhase                           :  time=78.60ms       mem=19.00MB
```

test-report.txt
```
Performance Comparison: 'bench_1-max.txt' vs 'bench_1-current-perf.txt'
================================================================================
:@EmitProgram                                   :  time=+2.90ms (+4.6%)           mem=+0.00MB (+0.0%)
:@GenerateProgram                               :  time=+4.67ms (+6.7%)           mem=0.00MB (0.0%)
:@GenerateProgram/OptimizeBytecode              :  time=0.00ms (0.0%)             mem=0.00MB (0.0%)
:@phases                                        :  time=+22.67ms (+2.6%)          mem=+0.00MB (+0.0%)
:@phases/AmbientLowering                        :  time=-0.07ms (-0.6%)           mem=0.00MB (0.0%)
```
