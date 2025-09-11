### **Issue**
_Please add a link to an issue_

### **Reason**
_Please add reason of the changes_

### **Description**
_Please add detailed description_

### **Tests**
```
ninja all tests
```
**All required pre-merge tests passed. Results are available in the internal CI.**

#### Functional Testing (Mandatory unless the change only involves text) [wiki](https://gitee.com/openharmony/arkcompiler_ets_frontend/wikis/%E4%BB%A3%E7%A0%81%E6%8F%90%E4%BA%A4%E8%A6%81%E6%B1%82%E5%8F%8A%E6%B5%8B%E8%AF%95%E9%AA%8C%E8%AF%81%E6%B5%81%E7%A8%8B)
1. es2abc Test Cases (Debug Mode)
- [ ] Passed
- [ ] Not applicable, no need to verify

2. Verifier Testing
- [ ] Passed
- [ ] Not applicable, no need to verify

3. 64-bit RK Compilation
- [ ] Passed
- [ ] Not applicable, no need to verify

4. Compile macOS SDK
- [ ] Passed
- [ ] Not applicable, no need to verify

#### Obfuscation Testing (Mandatory if ArkGuard-related changes are involved) [wiki](https://gitee.com/openharmony/arkcompiler_ets_frontend/wikis/%E6%B7%B7%E6%B7%86%E6%B5%8B%E8%AF%95%E9%AA%8C%E8%AF%81%E6%B5%81%E7%A8%8B?sort_id=11451209)
1. Unit Tests
- [ ] Passed
- [ ] Not applicable, no need to verify

2. Compiler Test Suite
- [ ] Passed
- [ ] Not applicable, no need to verify

3. TSC Extra Test Suite
- [ ] Passed
- [ ] Not applicable, no need to verify

4. Test262 Test Suite
- [ ] Passed
- [ ] Not applicable, no need to verify

5. Benchmark Tests
- [ ] Passed
- [ ] Not applicable, no need to verify

6. Automated Application Test Suite
- [ ] Passed
- [ ] Not applicable, no need to verify

7. Global Variable Check — Were any global variables created? If so, were they cleared?
- [ ] Global variables created and cleared
- [ ] Global variables created but not cleared
- [ ] No global variables created

#### Compatibility Testing (for instruction generation or file format changes)
1. Minor Version Compatibility Test <!-- If the change causes new ABC files to fail on older images, a new version number must be added -->
- [ ] Version number added
- [ ] Passed
- [ ] Not applicable, no need to verify

2. Major Version Compatibility Test <!-- When `target-api-version` is configured, the generated ABC must run on the corresponding version -->
- [ ] Passed
- [ ] Not applicable, no need to verify

3. es2abc Version Compatibility Test <!-- ABC files compiled by the new es2abc from an old API version should still be recognized and processed by the old es2abc -->
**Note: If the PR involves changes to version control test cases, you must also check its impact on other branches and confirm whether updates are needed in those branches.**
- [ ] Affected — Changes impact API/bytecode version and need to be synced to other branches
- [ ] Affected — Changes impact API/bytecode version but do not need to be synced to other branches
- [ ] Not affected — Changes do not involve API/bytecode version

4. Compatibility Testing for Lexical Environment Changes
**Note: If the PR involves modifications to the lexical environment, verify whether it is compatible with hot reload scenarios.**
- [ ] Involved, affects hot reload scenarios, requires impact assessment
- [ ] Involved, does not affect hot reload scenarios
- [ ] Not involved

#### Performance Testing (e.g., new syntax checking scenarios)
- [ ] Passed
- [ ] Not applicable, no need to verify

#### Command/abc format modification self-check, please contact the email below, synchronize to the relevant field
**Important: For modifications involving the abc2program, libpandafile and isa directories in the runtime_core repository, you must provide a compiled hap package for the helloworld project to the corresponding domain and contact the email address below.**
- [ ] Involved, already synchronized
- [ ] Not involved

**Email:** wutao185@huawei.com