# Host Regression Tests

Run the host tests from the repository root:

```sh
sh tests/run.sh
```

The runner compiles `test_*.c` with AddressSanitizer, UndefinedBehaviorSanitizer and strict warnings. Tests cover controller descriptors and reports, native ABI and timeout policies, pairing selection, controls configuration, input integration and transition recovery.

Fixtures and mocks support these host tests. Generated test executables go in `tests/build/` and are excluded from Git. Host checks do not replace PSP hardware testing.
