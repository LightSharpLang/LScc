## What is LSCC?
LScc is the L# compiler for Windows and Linux.

The L# Classic Compiler (LSCC) project was originally started by Julian Lavis--Fabbri in 2022.

No documentation on LSCC are available now. Any bugs or issues found with using LSCC should be reported at the 'issue' page of github. For help with LSCC, We have provided the command syntax and arguments list when no arguments are provided!

### Known Issues:


## Demo

```python

"using precompiled library"
with "stdlang.pclslib"

"using extern constant printf"
extern printf

def start(i):
  "comment"

  "defining variable b"
  b = 0
  "or"
  int b = 0
  "pointer to b"
  ptr ptr_b = 0
  ptr_b <- b

  "set one to the b"
  1 -> ptr_b

  "defining condition named condi_i"
  $condi_i = b == 1

  "using multiline-synax condition condi_i"
  $condi_i
    i + 2
    i - 1
  $

  "inline syntax"
  $condi_i$ i + 1
  i = 8

  return(i)
end
```

## License

The project, past and future releases and code, is under the CreativeCommons License:

<div style="display:flex;flex-direction:row;">
  <img src="https://mirrors.creativecommons.org/presskit/buttons/88x31/png/by-nc-sa.png" width="200px" style="float:right;"/>
  <img src="https://mirrors.creativecommons.org/presskit/buttons/88x31/png/by-nc-sa.eu.png" width="200px" style="float:left;"/>
</div>
