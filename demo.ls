"note that this file doesn't compile, the issue will be found and fixed! thank you"
"demo file - not usable"

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
  ptr_b = 0
  ptr_b <- b

  "defining condition named condi_i"
  $condi_i = i == 1

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
