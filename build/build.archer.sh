module load cmake
module load gcc
module switch PrgEnv-cray PrgEnv-gnu
cmake ../src/ -DCMAKE_TOOLCHAIN_FILE=ARCHER-toolchain.cmake
make -j10
make install
