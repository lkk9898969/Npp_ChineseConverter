cd .\OpenCC
start "" "cmd" /C cmake -S. -Bbuild -DCMAKE_INSTALL_PREFIX:PATH=. -DBUILD_SHARED_LIBS=OFF
cd ..\libarchive
cmake -Bbuild -G"Visual Studio 17 2022"