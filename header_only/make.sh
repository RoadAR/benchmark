#!/bin/bash

# Generate H-only file from hpp
cat ../include/roadar/benchmark.hpp > rbenchmark.hpp

# Append cpp with remove unnecessary include
grep -v "\<roadar\/benchmark.hpp\>" ../src/benchmark.cpp >> ./rbenchmark.hpp