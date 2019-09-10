#git pull origin master
cd ../build
cmake ../
make
make test
sudo make install
sudo ldconfig
cd ../apps
