git pull origin 1909_experiment
cd ../build
cmake ../
make
make test
sudo make install
sudo ldconfig
cd ../apps
