all: myht correctness dcas

myht: perfTest.cpp fg_hash_table.h seq_hash_table.h del_opt_hash_table.h haz_ptr_hash_table.h tools/linked_list_node.h
	g++ -std=c++11 -I../common -I./tools/cds-2.2.0/build/include perfTest.cpp -o myht -lpthread -L./tools/cds-2.2.0/build/lib64 -lcds -fno-strict-aliasing

correctness: correctnessTest.cpp fg_hash_table.h seq_hash_table.h del_opt_hash_table.h haz_ptr_hash_table.h tools/linked_list_node.h
	g++ -std=c++11 -I../common -I./tools/cds-2.2.0/build/include correctnessTest.cpp -o correctness -lpthread -L./tools/cds-2.2.0/build/lib64 -lcds -fno-strict-aliasing

dcas: x86Test.cpp dcas_hash_table.h seq_hash_table.h tools/linked_list_node.h tools/dcas_node.h
	g++ -m32 -I../common x86Test.cpp -o dcas -lpthread

clean:
	rm -f *.o perf correctness dcas
