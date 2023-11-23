"note that this file doesn't compile, the issue will be found and fixed! thank you"
"this demo file has a lot of warnings due to the demonstration os the language's capability"
"you can remove the types warnings with --w-type"
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
