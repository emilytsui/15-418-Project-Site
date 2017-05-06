all: myht correctness hazptr dcas

myht: perfTest.cpp fg_hash_table.h seq_hash_table.h mem_leak_hash_table.h haz_ptr_hash_table.h tools/linked_list_node.h
	g++ perfTest.cpp -o myht -lpthread

correctness: correctnessTest.cpp fg_hash_table.h seq_hash_table.h mem_leak_hash_table.h haz_ptr_hash_table.h tools/linked_list_node.h
	g++ correctnessTest.cpp -o correctness -lpthread

hazptr: hazPtrTest.cpp haz_ptr_hash_table.h seq_hash_table.h tools/linked_list_node.h
	g++ -std=c++11 -I./tools/cds-2.2.0/build/include hazPtrTest.cpp -o hazptr -lpthread -L./tools/cds-2.2.0/build/lib64 -lcds -fno-strict-aliasing

dcas: x86Test.cpp dcas_hash_table.h seq_hash_table.h tools/linked_list_node.h tools/dcas_node.h
	g++ -std=c++11 -m32 -mcx16 x86Test.cpp -o dcas -lpthread

clean:
	rm -f *.o perf correctness hazptr dcas
