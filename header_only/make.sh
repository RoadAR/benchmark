#!/bin/bash

# Generate H-only file from hpp
sed "s/define R_FUNC/define R_FUNC static/g" ../include/roadar/benchmark.hpp > rbenchmark.hpp

# Append cpp with remove unnecessary include
grep -v "\<roadar\/benchmark.hpp\>" ../src/benchmark.cpp >> ./rbenchmark.hpp