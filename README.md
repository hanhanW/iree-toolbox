# iree-toolbox
IREE tools that are useful but not upstreamed

## verify_affinity_dot

A tool to verify if the input program has valid affinity or not. The checks are
soft, so the program could be invalid even if it passes the check. The input is
expected to be a dot file dump from `iree-compile` with
`--iree-flow-dump-dispatch-graph` flag.

Complie:

```
clang++ --std=c++17 verify_affinity_dot.cpp -o verify_affinity_dot
```

Example:

```
# Usage: ./verify_affinity_dot [any_string_to_enable_log]

iree-compile model.mlir \
  --iree-hal-target-device='hip[0]' \
  --iree-hal-target-device='hip[1]' \
  --iree-hip-target=gfx942 \
  -o /tmp/out.vmfb \
  --iree-flow-dump-dispatch-graph \
  --iree-flow-dump-dispatch-graph-output-file=/tmp/model.dot

# Verify without logs.
./verify_affinity_dot < /tmp/model.dot

# Verify with logs.
./verify_affinity_dot log < /tmp/model.dot
```
