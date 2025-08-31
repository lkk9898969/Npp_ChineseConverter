%~d0
cd %~dp0
cd .\OpenCC
start "" "cmd" "/C cmake -S. -Bbuild -DCMAKE_INSTALL_PREFIX:PATH=. -DBUILD_SHARED_LIBS=OFF && cmake --build build --config Release --target Dictionaries"
cd ..\liblzma
cmake -Bbuild -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=build
cmake --build build --config Release --target install
cd ..\libarchive
cmake -Bbuild -G"Visual Studio 17 2022" -DCMAKE_PREFIX_PATH=../liblzma/build