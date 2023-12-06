from urllib import request
from sys import argv
from subprocess import call as cmd
import os
from shutil import unpack_archive as zopen
from shutil import copy
from shutil import rmtree as rmdir

def nop():
  return

if len(argv) < 2:
  print("you need to pass arguments")
  exit()

if argv[1].lower() == "install":
  if len(argv) > 2:
    print("installing", argv[2])
    [[(os.remove(i) if i.endswith(".cpp") else nop()) for i in k] for s, d, k in os.walk(os.curdir)]
    [[(os.remove(i) if i.endswith(".h") else nop()) for i in k] for s, d, k in os.walk(os.curdir)]
    url = "https://github.com/LightSharpLang/LScc/archive/refs/tags"
    u = request.urlopen("https://github.com/LightSharpLang/LScc/releases/"+argv[2])
    url = url + u.geturl()[u.geturl().rfind("/"):] + ".zip"
    print("downloading source code from github...")
    with open("code.zip", "wb") as f:
      f.write(request.urlopen(url).read())
    print("done")
    print("extracting source...")
    zopen('code.zip', os.curdir)
    os.remove("code.zip")
    dir = os.listdir()[1]
    print("changing directory to", dir)
    os.chdir(dir)
    cmd(["g++", "token.cpp", "compilation.cpp", "LScc.cpp", "Error.cpp", "escString.cpp", "-o", "lscc", "--std=c++17"])
    copy("lscc", "../lscc")
    print("removing sources...")
    rmdir("../"+dir)
    print("done")
    print("now put nasm into", os.curdir)
  else:
    print("installing current")
    cmd(["g++", "token.cpp", "compilation.cpp", "LScc.cpp", "Error.cpp", "escString.cpp", "-o", "lscc", "--std=c++17"])
else:
  print("unrecognized option", argv[1])

