# ir Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Defines the static frontend AST, node mappings, visitors, source dumps, and metadata. This tree is independent of the dynamic frontend IR.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/ir/astNode.h` | Base node contract and traversal | High: compiler/consumer contract |
| `ets2panda/ir/astNodeMapping.h` | Node-kind/class mappings | High: compiler/consumer contract |
| `ets2panda/ir/astNodeFlags.h` | Node flags | High: compiler/consumer contract |
| `ets2panda/ir/ets/` | Static-language node classes | High: compiler/consumer contract |
| `ets2panda/ir/visitor/` | Visitor infrastructure | High: compiler/consumer contract |
| `ets2panda/ir/srcDump.cpp` | Source dumping | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Node representation and consumers | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |
| Invariants | `docs/knowledge/ets2panda/AST_Verifier_Knowledge_Base.md` |
| Exported node APIs | `docs/knowledge/ets2panda/Public_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not share dynamic-route node assumptions with this tree.
- Do not create structural rewrites in checker/varbinder; use lowering and its rebind/recheck protocol.
- Do not add node flags as a workaround for type logic.
- Preserve parent links, ranges, node ownership, visitor coverage, and metadata consistency.
- Update parser, checker/binder, lowering, verifier, dumping, and codegen consumers when introducing a node kind.
- Use existing arena allocation and clone/traversal APIs; do not introduce manual ownership.
- Edit codegen templates and source headers rather than generated public API output.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

Build the affected static frontend targets from the configured static build directory. Update both `ets2panda/CMakeLists.txt` and `ets2panda/BUILD.gn` source registration where applicable. Run relevant parser/AST verifier cases, `ets2panda/test/unit/public/` invariants, and runtime tests for altered output. Follow the static KB for runner working directories and expected comparisons. Dump structure, source ranges, bindings, and runtime behavior are separate acceptance gates.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
