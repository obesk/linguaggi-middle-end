# Variables
CC = clang
OPT = opt
CFLAGS = -emit-llvm -O0 -Xclang -disable-O0-optnone -S
SRCS = test_LICM.c test_localopts.c test_loopfusion.c

# Passes corresponding to each source file
PASS_test_LICM = mylicm
PASS_test_localopts = localopts
PASS_test_loopfusion = myloopfusion

# Default target
all: $(SRCS:%.c=%.optimized) $(SRCS:%.c=%.original)

# Rule to generate the initial LLVM IR without memory optimizations
%.nomem.ll: %.c
	$(CC) $(CFLAGS) $< -o $@

# Rule to apply mem2reg optimization
%.ll: %.nomem.ll
	$(OPT) -p mem2reg $< -S -o $@

# Rule to apply the corresponding custom optimization pass
%.optimized.ll: %.ll
	$(OPT) -p $(PASS_$(*F)) $< -S -o $@

# Rule to compile the optimized LLVM IR to an executable
%.optimized: %.optimized.ll
	$(CC) $< -o $@

%.optimized: %.optimized.ll
	$(CC) $< -o $@

%.original: %.c
	$(CC) $< -o $@

.PRECIOUS: %.ll %.optimized.ll

# Clean up generated files
clean:
	rm -f *.ll *.original *.optimized

.PHONY: all, clean



# # Variables
# CC = clang
# OPT = opt
# CFLAGS = -emit-llvm -O0 -Xclang -disable-O0-optnone -S
# SRCS = test_LICM.c #test_localopts.c test_loopfusion.c

# # Passes corresponding to each source file
# PASS_test_LICM = mylicm
# PASS_test_localopts = localopts
# PASS_test_loopfusion = myloopfusion

# # Default target
# all: $(SRCS:%.c=%.optimized)

# # Rule to generate the initial LLVM IR without memory optimizations
# %.nomem.ll: %.c
# 	$(info  generating nomem)
# 	$(CC) $(CFLAGS) $< -o $@

# # Rule to apply the corresponding custom optimization pass
# %.ll: %.nomem.ll
# 	$(OPT) -p mem2reg $< -S -o $@

# %.optimized.ll: %.optimized.ll
# 	$(OPT) -p $(PASS_$(*F)) $< -S -o $@

# # Rule to compile the optimized LLVM IR to an executable
# %.optimized: %.optimized.ll
# 	$(CC) $< -o $@

# # Clean up generated files
# clean:
# 	rm -f *.nomem.ll *.mylicm.ll *.optimized

# .PHONY: all clean
