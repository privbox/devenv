export PRIVBOX_PREFIX=${WORKDIR?}
export CC_RAW=${PRIVBOX_PREFIX}/llvm-project/build/bin/clang
export CFLAGS_NOINSTR="-g -O2 -mrelax-all"
export CFLAGS_INSTR="-g -O2 -mllvm -x86-priv-san-boundary=16 -mrelax-all -mllvm -enable-priv-san-branching -mllvm -priv-san-align-bytes=16"
export CFLAGS_FULLINSTR="-g -O2 -mllvm -x86-priv-san-boundary=32 -mrelax-all -mllvm -enable-priv-san"

export MUSL_NOINSTR_DIR=${PRIVBOX_PREFIX}/musl-noinstr/
export MUSL_INSTR_DIR=${PRIVBOX_PREFIX}/musl-instr/
export MUSL_FULLINSTR_DIR=${PRIVBOX_PREFIX}/musl-fullinstr/

export MUSL_NOINSTR_PREFIX=${MUSL_NOINSTR_DIR}/prefix
export MUSL_INSTR_PREFIX=${MUSL_INSTR_DIR}/prefix
export MUSL_FULLINSTR_PREFIX=${MUSL_FULLINSTR_DIR}/prefix


export CC_MUSL_NOINSTR=${MUSL_NOINSTR_PREFIX}/bin/musl-clang
export CC_MUSL_INSTR=${MUSL_INSTR_PREFIX}/bin/musl-clang
export CC_MUSL_FULLINSTR=${MUSL_FULLINSTR_PREFIX}/bin/musl-clang

export DEVENV_PROJ=${PRIVBOX_PREFIX}/devenv/bin
export PRIVBOX_KERN_HEADERS=${PRIVBOX_PREFIX}/devenv/images/build/kernel-headers
