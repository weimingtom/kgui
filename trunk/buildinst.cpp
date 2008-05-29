#if 0

for debug build ( windows )

cd ffmpeg
./configure --enable-shared --disable-static --enable-memalign-hack
make clean
make
copy libavcodec\avcodec-51.lib ..\wiffmeg
copy libavcodec\avcodec-51.dll ..\wiffmeg
copy libavformat\avformat-51.lib ..\wiffmeg
copy libavformat\avformat-51.dll ..\wiffmeg
copy libavutil\avutil-49.lib ..\wiffmeg
copy libavutil\avutil-49.dll ..\wiffmeg
cd ..

for release build ( windows )
cd ffmpeg
./configure --enable-memalign-hack
make clean
make
copy libavcodec\libavcodec.a ..\wiffmeg
copy libavformat\libavformat.a ..\wiffmeg
copy libavutil\libavutil.a ..\wiffmeg
cd ..




#endif