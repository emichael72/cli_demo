# CLI Engine

## Features

1. Familiar `argc`, `argv` C-style function invocation with argument parsing.
2. Ability to inject multiple CLI command tables from various modules.
3. Commands are sorted to speed up searches at runtime.
4. Command name auto-completion using the Tab key.
5. Automatic 'help' generation.
6. Optional local echo support.

## Building

To build the project, simply run:

```

make

```

## Supported Platforms

The code compiles and runs on **Linux**.

## RTOS Ports

The thread running the CLI engine is designed to mimic a typical scheduler as closely as possible.