# QL-Picture

A quick and dirty anyformat (that imagemagick supports) to QL _SCR or _PIC converter.

##

Debian/Ubuntu dependencies are `libmagickwand-dev` and `imagemagick` other distros adjust as required!

```
git clone --recursive https://github.com/xXorAa/ql-picture
cd ql-picture
make

```

## Running

Below is the arguments, the output is `<file>_scr`.

```
❯ ./build/ql-picture -h
Usage: ql-picture [OPTIONS] <file>

Options:
  -h,--help            Print this help message and exit
  -m,--mode            QL mode 4/8 default(8)
  -f,--format          format pic/scr default(scr)
  -v,--version         version
```
