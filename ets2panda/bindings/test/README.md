### How to run bindings test?

first, you need download a SDK package, then unzip ets component into `bindings/test/` directory.
Download SDK package from [here](http://ci.openharmony.cn/workbench/cicd/dailybuild/dailylist).

```Bash
unzip /path/to/ets-xxx.zip -d /path/to/bindings/test/
cd /path/to/bindings

### run test
npm install
npm run test
```

#### tips
1. If you want to update a lot of expected results, you can use `npm run test:update` to update all expected results.

### testcase directory structure
.
├── cases.ts
├── run_tests.ts
├── expected
│   ├── exampleFuncName.json
└── testcases
    ├── .idea
    │   └── .deveco
    │       ├── exampleFuncName
    │       │   └── arktsconfig.json
    │       ├── lsp_build_config.json
    │       └── lsp_compileFileInfos.json
    └── exampleFuncName
        └── exampleFuncName1.ets

case.ts:
```typescript
{
   testName: {
      "expectedFilePath": "/path/to/expected.json",
      "1": [ "param1", "param2" ], // lsp will call lsp.testName(param1, param2)
      "2": [ "param1", "param2" ]
   }
}
```

#### How to add a new test case?
1. add exampleFuncName2.ets file in `testcases/exampleFuncName` directory
2. add parameters in `cases.ts` file
3. add expected result in `expected/exampleFuncName.json` file

#### How to add a new test function?
1. add exampleFuncName2 directory in `testcases` directory
2. add exampleFuncName2 field in `cases.ts` file
3. add exampleFuncName2.json in `expected` directory
4. add a new test case according to the above steps
