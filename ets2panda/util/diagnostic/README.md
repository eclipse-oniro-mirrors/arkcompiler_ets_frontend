# Diagnostic YAMLs
These files encode the various diagnostic messages that es2panda might emit.

## Adding new diagnostics
When adding a new diagnostic, adding them at the end *guarantees* conflicts between PRs.  We avoid that by keeping the lists sorted by name.

Keeping them sorted still doesn't solve the issue of two PRs trying to add diagnostics with the same id.  We mostly solve that by *not* choosing the numbers by hand, nor just incrementing them, but by generating them randomly.

Both are achieved by running `ets_frontend/ets2panda/scripts/normalize_yaml somefile.yaml`, where `somefile.yaml` is the one you have added new messages to.

You can also run `normalize_yaml --all` to normalize all diagnostic YAMLs.

## Deleting diagnostics
To avoid accidentally re-using old ids, please move them into the `graveyard` list at the end of the file.  These are forever forbidden from being emitted by the compiler.

See `semantic.yaml` for an example of how it should look.

## Checking
YAMLs are checked at build time in `ets_frontend/ets2panda/util/diagnostic/diagnostic.rb`.