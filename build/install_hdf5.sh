#!/usr/bin/env bash
set -e

if [ ${TARGET} = "win64" ]; then
    sudo apt-get -y install mxe-${WINTARGET}-hdf5
else
    if [ ${TRAVIS_OS_NAME} = "linux" ]; then
        echo installing HDF5 with source
        wget https://www.hdfgroup.org/ftp/HDF5/prev-releases/hdf5-1.8.17/src/hdf5-1.8.17.tar.bz2
        tar -xjf hdf5-1.8.17.tar.bz2
        cd hdf5-1.8.17
        ./configure
        make CFLAGS="-w -fPIC" CXXFLAGS="-w -fPIC"
        sudo make install
        cd ../
    else
        echo installing HDF5 from Homebrew
        sudo brew install homebrew/science/hdf5 --universal
    fi
fi
