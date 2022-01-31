# garrett

`garrett` is an evaluation tool for [gatekit](https://github.com/sat-clique/gatekit).

## Building `garrett`

Clone this repository and run

```
git submodule init
git submodule update
```

`garrett` has only a single runtime dependency: `libarchive`.

This tool can be built like a regular CMake project:

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release <path-to-garret-repo>
make -j
```

## Running `garrett`

After building garrett, run `bin/garrett <CNFFile>`. The tool will scan the gate structure of the problem instance in `<CNFFile>` and print some metrics.
`<CNFFile>` may be compressed with any format supported by libarchive.

