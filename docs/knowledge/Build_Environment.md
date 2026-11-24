# Build and Test Path Variables

All AGENTS and knowledge-base command recipes use the following path contract. Set applicable variables in your shell to actual absolute paths from the current checkout/build configuration before running a recipe. Do not paste a placeholder as a path or commit developer-specific settings.

| Variable | Meaning | Required evidence |
|---|---|---|
| `OH_ROOT` | OpenHarmony checkout containing `build.sh` and `arkcompiler/` | Checkout and frontend directory exist |
| `FRONTEND_BUILD_DIR` | Dynamic frontend output containing `es2abc` | Correct host/product/configuration |
| `STATIC_BUILD_DIR` | Configured static CMake/Ninja output | Correct compiler/runtime configuration |

## 1. Source root check

```bash
: "${OH_ROOT:?Set OH_ROOT to the absolute OpenHarmony checkout root}"
case "$OH_ROOT" in /*) ;; *) echo "OH_ROOT must be absolute" >&2; exit 1 ;; esac
test -f "$OH_ROOT/build.sh"
test -d "$OH_ROOT/arkcompiler/ets_frontend"
```

Do not proceed to build/test commands if any applicable path check fails. Run the following checks as well for the selected route. Before the first build, a target binary may not exist: validate the chosen output configuration, build it, then check the executable before running tests.

## 2. Dynamic frontend output check

```bash
: "${FRONTEND_BUILD_DIR:?Set FRONTEND_BUILD_DIR for the selected dynamic build}"
case "$FRONTEND_BUILD_DIR" in /*) ;; *) echo "FRONTEND_BUILD_DIR must be absolute" >&2; exit 1 ;; esac
test -x "$FRONTEND_BUILD_DIR/es2abc"
"$FRONTEND_BUILD_DIR/es2abc" --help
```

## 3. Static build output check

```bash
: "${STATIC_BUILD_DIR:?Set STATIC_BUILD_DIR to the configured static build}"
case "$STATIC_BUILD_DIR" in /*) ;; *) echo "STATIC_BUILD_DIR must be absolute" >&2; exit 1 ;; esac
test -f "$STATIC_BUILD_DIR/CMakeCache.txt"
```

A build directory existing is not proof it matches the source revision, host, runtime, or intended options. Check its configuration and artifacts. Use Ninja recipes only when that generator is configured.

## 4. Working directories

| Operation | Working directory / invocation |
|---|---|
| OpenHarmony build | `cd "$OH_ROOT"`, then `./build.sh` |
| Dynamic regression / Test262 | `cd "$OH_ROOT/arkcompiler/ets_frontend"` |
| Static CMake build | `cmake --build "$STATIC_BUILD_DIR"` |
| Static Ninja targets | `ninja -C "$STATIC_BUILD_DIR"` |
| Static runner | Absolute script path under `OH_ROOT`; configured runner environment required |
| npm build/tests | Named package directory under the frontend checkout |

Use route-specific guides for runtime libraries, SDKs, fixtures, target versions, and acceptance criteria. Recipes are parameterized on purpose: machine-specific paths stay local, and every recipe must validate its applicable inputs using sections 1–3 before execution.
