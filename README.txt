Compile:
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j

Run:
  ./tablefs -mountdir [MOUNT DIR] -metadir [METADATA DIR] -datadir [DATA DIR]
