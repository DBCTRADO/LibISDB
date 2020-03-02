#!/bin/bash

git_hash=$(git rev-parse --short HEAD)

if [ $? -eq 0 ]
then
    grep -q "$git_hash" LibISDBVersionHash.hpp || echo -e -n "#define LIBISDB_VERSION_HASH_ \"${git_hash}\"\r\n" >LibISDBVersionHash.hpp
else
    rm -f LibISDBVersionHash.hpp
fi
