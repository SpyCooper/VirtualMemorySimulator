CC=gcc
CXX=g++
CFLAGS=-Wall -Werror -O2
CXXFLAGS=${CFLAGS}

default: vm

vm:
	${CXX} ${CXXFLAGS} -o vm vm.cc

.PHONY: vm

clean: 
	rm -f *.o vm .test.results
	rm -rf results

.PHONY: test
test: vm
	chmod +rx test testoptimal
	rm -f .test.results
	-./test input.0.psize4k 
	-./test input.0.psize5
	-./test input.0.psize10
	-./test input.0.psize1
	-./test input.1.easy
	-./test input.1.eachstep
	-./test input.1.lru
	-./test input.2.only1frame
	-./test input.handout
	-./test input.b.p440
	-./test input.b.p442
	-./test input.b.p443
	-./test input.b.belady1
	-./test input.b.belady2
	-./test input.o.optimal
	-./test input.w.bs
	-./test input.w.disk
	-./test input.w.ondisk_test
	-./test input.9.bigrandom
	-./testoptimal
	-echo "Test results: "; cat .test.results
