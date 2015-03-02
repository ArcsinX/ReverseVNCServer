#!/bin/bash
cd libs
for i in ./*; do
  if [[ -d $i && -e $i/reversevncserver ]];then
    mkdir -p ../app/src/main/jniLibs/$i
    cp $i/reversevncserver ../app/src/main/jniLibs/$i/libreversevncserver.so;
  fi
done
