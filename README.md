# akifiletable - Filetable Builder for AKI Corporation's Nintendo 64 games
`akifiletable` is a file table builder for AKI Corporation's Nintendo 64 games.

## Usage
Running `akifiletable` without any arguments will print the program usage.

```bash
akifiletable -l LIST_FILE [-o OUTPUT_FILETABLE] [-i OUTPUT_INDEX] [-h OUTPUT_HEADER] [-a OUTPUT_INCLUDE] [-v] [-d]
```

### Required arguments
    -l LIST_FILE    JSON file containing list of files to put in the filetable.

### Optional arguments
    -o OUTPUT_FILETABLE    Filename for filetable data. Defaults to `filetable.bin` if omitted.
    -i OUTPUT_INDEX        Filename for filetable index. Defaults to `filetable.idx` if omitted.
	-h OUTPUT_HEADER       Filename for filetable symbol header. Defaults to `filetable.h` if omitted.
	-a OUTPUT_INCLUDE      Filename for filetable linker symbol file. Defaults to `filetable.txt` if omitted.
	-v                     Verbose mode; output information about each file entry. Will likely inflate build times.
	-d                     Write filetable symbol header file only.

## List File JSON Format
The list file is a JSON file consisting of a single array `[]` of entries.

Each entry has the following format:
```json
{
  "file":"path",
  "lzss":false,
  "symbol":"SOMEFILE",
  "exportsize":false,
  "exportsizepad":0,
}
```

### `file`
Path to the file to add to the filetable.

### `lzss`
A boolean (`true`/`false`) determining if the corresponding entry in the file
table index will have the least significant bit set. This only signifies that
the file is LZSSed, and *will not* encode the file for you.

### `symbol`
An optional string value to be exported. This is prefixed with `FILEID_` in
the output header file.

### `exportsize`
A boolean (`true`/`false`) determining if a symbol with the file's (decoded)
size should be exported. If this is `true`. `symbol` **MUST** be set to
a non-empty value.

### `exportsizepad`
Number of bytes to pad the export size entry. For this to have any effect,
`symbol` must exist and be non-empty, and `exportsize` must be set to `true`.

## License
This program is licensed under the Unlicense. See the `UNLICENSE` file for details.

This program uses [Frozen](https://github.com/cesanta/frozen/), which is licensed
under the Apache 2.0 license.
