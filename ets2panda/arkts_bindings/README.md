# ArkTS Bindings

This is an ArkTS library that allows to use `es2panda` plugin API in `ArkTS` language. The library API is implemented as laungage bindings using the `ANI` library.

## Build
CMake target:
```bash
ninja arkts_bindings
```

## Tests:
Tests are located in `ets_frontend/ets2panda/test/arkts_bindings`.

Run tests using following CMake target:
```bash
ninja arkts_bindings_tests
```