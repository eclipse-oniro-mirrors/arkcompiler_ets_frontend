# Diagnostic YAMLs
The YAML files in this directory (and scattered in a few others) encode the various diagnostic messages that es2panda might emit.

Are you about to contribue a [new diagnostic](#adding-new-diagnostics), or [delete](#deleting-already-committed-diagnostics) or [modify](#modifying-an-existing-diagnostic) an existing one?  Jump to the ["Contributing" section](#contributing)!

## Validation
YAMLs are validated at build time in `ets_frontend/ets2panda/util/diagnostic/diagnostic.rb`. 

Additional linting is done by running `ets_frontend/ets2panda/scripts/lint_yaml` manually or by running it on the CI.

## Normalizing
After any YAML modification, run either of these:
```bash
# to normalize all diagnostic YAMLs:
ets_frontend/ets2panda/scripts/normalize_yaml --all
```

```bash
# to only normalize a specific file:
ets_frontend/ets2panda/scripts/normalize_yaml /path/to/file.yaml
```

## Contributing
## Install Python dependencies
You might need to run `scripts/install-deps-ubuntu -i=test` if you haven't done so in a while.

Or you can run `pip install numpy ruamel.yaml`.

### Adding new diagnostics
Add them at the end, before the `graveyard` (if one exists).  Do not add an `id` field.  Then run `normalize_yaml`, as [described above](#normalizing).

```yaml
...
- name: YIELD_IN_GENERATOR_PARAM
  id: 54
  message: "Yield is not allowed in generator parameters."

- name: MY_NEW_DIAGNOSTIC
  message: Oops, something happened
  # *NO* `id` field!

graveyard:
- 317
# See ets_frontend/ets2panda/util/diagnostic/README.md before contributing.
```

### Deleting already committed diagnostics
Copy the `id` to the `graveyard`, then delete the diagnostic entry, then run [run `normalize_yaml`](#normalizing) to sort the `graveyard` entries.

See `semantic.yaml` for an example of how it should look.

```yaml
- name: YIELD_IN_GENERATOR_PARAM
  id: 54
  message: "Yield is not allowed in generator parameters."

# Removed diagnostic:
# - name: MY_NEW_DIAGNOSTIC
#   id: 12345
#   message: Oops, something happened

graveyard:
- ...
- 12345 # copied from the removed diagnostic
# See ets_frontend/ets2panda/util/diagnostic/README.md before contributing.
```

### Modifying an existing diagnostic
If you only modify the `message`, then no other action is needed.  If you rename it (modifying the `name` field), then you need to run `normalize_yaml` again.

Do not modify the `id`.  If you do, add the old value to the `graveyard`, same as if you were [deleting it](#deleting-already-committed-diagnostics).

## Format
### Console
Diagnostics emitted by frontend tools follow this format:

```
[file_name:line:column] Type PRE01234 Long description of error
```

Where `Type` is the type of diagnostic (eg.: semantic error, warning), `PRE` is a [kind prefix](#prefixes) and `01234` is a numeric identifier that is unique among diagnostics of a kind, but not necessarily between diagnostics of different kinds.

A concrete example:
```
[export_anonymous_with_object_expreesion.ets:16:16] Semantic error ESE0174: Cannot infer type for gensym%%_anonymous_const because class composite needs an explicit target type
```

### Prefixes
Frontend tools (like `es2panda`) prepend these prefixes to the numeric identifier of the diagnostic message:

- F: fatal
- ESY: syntax
- ESE: semantic
- W: warning
- WP: plugin warning
- EP: plugin error
- ED: declgen ets2ts error
- WD: declgen ets2ts warning
- EID: isolated declgen
- EAC: ArkTS config error
- S: suggestion

### YAML
The format in the YAML files is mostly self-explanatory, but see the [quickstart section below](#contributing) before making changes.

```yaml
- name: INVALID_TUESDAY_WIDGET
  id: 12345
  message: "Widget '{}' of type '{}' is only valid on Tuesdays"
```

The `id` is the unique identifier that the user will see, the `name` is the unique name for the generated C++ object.  The prefix is not included here.

The `message` is a string with any number of placeholders (`{}`) that are substituted at run time.  The apostrophe/single-quote is not part of the placeholder.


## Background
Adding new diagnostics at the end of diagnostic YAMLs used to result in lots of Git conflicts, because everyone was trying to modify the same part of the same file.

This first problem was mostly solved by sorting diagnostics by their name.

That still left the problem of unique but human-friendly identifiers, which was (mostly) solved by generating IDs randomly.  `normalize_yaml` has an explanation of the math behind this.

The deleted ids were also not tracked, so the graveyard was added.  It is also kept sorted to minimize conflicts and to make it easier to find ids.