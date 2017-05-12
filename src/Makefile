all: myht correctness hazptr dcas loadfactor dcasTime

myht: perfTest.cpp fg_hash_table.h seq_hash_table.h mem_leak_hash_table.h tools/linked_list_node.h
	g++ perfTest.cpp -o myht -lpthread

correctness: correctnessTest.cpp fg_hash_table.h seq_hash_table.h mem_leak_hash_table.h tools/linked_list_node.h
	g++ correctnessTest.cpp -o correctness -lpthread

hazptr: hazPtrTest.cpp haz_ptr_hash_table.h seq_hash_table.h tools/linked_list_node.h
	g++ -std=c++11 -I./tools/cds-2.2.0/build/include hazPtrTest.cpp -o hazptr -lpthread -L./tools/cds-2.2.0/build/lib64 -lcds -fno-strict-aliasing

dcas: x86Test.cpp dcas_hash_table.h seq_hash_table.h tools/linked_list_node.h tools/dcas_node.h
	g++ -std=c++11 -m32 -mcx16 x86Test.cpp -o dcas -lpthread

dcasTime: x86CompTiming.cpp dcas_hash_table.h seq_hash_table.h tools/linked_list_node.h tools/dcas_node.h
	g++ -std=c++11 -m32 -mcx16 x86CompTiming.cpp -o dcasTime -lpthread

loadfactor: loadFactorTest.cpp fg_hash_table.h seq_hash_table.h mem_leak_hash_table.h tools/linked_list_node.h
	g++ -std=c++11 -I./tools/cds-2.2.0/build/include loadFactorTest.cpp -o loadfactor -lpthread -L./tools/cds-2.2.0/build/lib64 -lcds -fno-strict-aliasing


clean:
	rm -f *.o perf correctness hazptr dcas loadfactor dcasTime
