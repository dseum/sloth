rm -rf tests
mkdir -p tests

make
.build/exe generate tests/poisson_ 1 2
.build/exe process tests/poisson_1 tests/poisson_2

echo
echo "poisson_1"
du -h tests/poisson_1
du -h tests/poisson_1.sloth

echo
echo "poisson_2"
du -h tests/poisson_2
du -h tests/poisson_2.sloth
