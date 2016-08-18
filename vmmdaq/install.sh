#!/bin bash

cd include/

rootcling -f vector_dict.cxx -c vector_dict.h LinkDef.h
#rootcint -f vector_dict.cxx -c vector_dict.h LinkDef.h

mkdir ../build/objects
g++ -o ../build/objects/libRootVectorLib.so vector_dict.cxx `root-config --cflags --libs` -shared -fPIC

cd ../build
ln -s objects/libRootVectorLib.so .

