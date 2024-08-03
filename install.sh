#!/bin/bash

COMMON="$(pwd)/COMMON"
LLVM="../SRC/llvm-project-llvmorg-17.0.6/llvm"

# creazione dei file comuni
ln -sf "$COMMON/CMakeLists.txt" "$LLVM/lib/Transforms/Utils"
ln -sf "$COMMON/PassBuilder.cpp" "$LLVM/lib/Passes"
ln -sf "$COMMON/PassRegistry.def" "$LLVM/lib/Passes"

# creazione dei sorgenti per gli assignment
ASSIGNMENTS=("ASSIGNMENT1" "ASSIGNMENT3" "ASSIGNMENT4")

for assignment in "${ASSIGNMENTS[@]}"; do
    # file h
    for file in "$(pwd)/$assignment"/*.h; do
        ln -sf $file "$LLVM/include/llvm/Transforms/Utils"
    done
    # file cpp
    for file in "$(pwd)/$assignment"/*.cpp; do
        ln -sf $file "$LLVM/lib/Transforms/Utils"
    done
done