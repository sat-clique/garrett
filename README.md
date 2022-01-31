# garrett

`garrett` is an evaluation tool for [gatekit](https://github.com/sat-clique/gatekit).

## Building `garrett`

Clone this repository and run

```
git submodule init
git submodule update
```

`garrett` has only a single runtime dependency: `libarchive`.

`garrett` is a regular CMake project. To build it, run

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release <path-to-garret-repo>
make -j
```

## Running `garrett`

After building garrett, run `bin/garrett <CNFFile>`. The tool will run
`gatekit`'s gate structure scanner on the given CNF file and and print some metrics.
`<CNFFile>` may be compressed in any format supported by `libarchive`.

