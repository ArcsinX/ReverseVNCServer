#!/bin/bash
for i in libs/*; do
  if [[ -d $i && -e $i/reversevncserver ]];then
    cp $i/reversevncserver $i/libreversevncserver.so;
  fi
done
