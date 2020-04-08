#!/usr/bin/python3
import sys
from compileall import compile_dir
from os import environ
from os import path

destdir = environ.get("DESTDIR", "")
sitelib = sys.argv[1]

print("Compiling python bytecode...")
compile_dir(destdir + path.join(sitelib), optimize=1)
