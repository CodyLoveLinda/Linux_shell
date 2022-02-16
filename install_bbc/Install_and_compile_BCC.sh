git clone https://github.com/iovisor/bcc.git
mkdir bcc/build; cd bcc/build
cmake ..
make
sudo make install
cmake -DPYTHON_CMD=python3 .. # build python3 binding
pushd src/python/
make
sudo make install
popd

#install python
sudo apt install python-is-python3
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 150
