all: myht correctness dcas

perf: perfTest.cpp fg_hash_table.h seq_hash_table.h tools/linked_list_node.h
	g++ -I../common perfTest.cpp -o perf -lpthread

correctness: correctnessTest.cpp fg_hash_table.h seq_hash_table.h tools/linked_list_node.h
	g++ -I../common correctnessTest.cpp -o correctness -lpthread

dcas: x86Test.cpp dcas_hash_table.h seq_hash_table.h tools/linked_list_node.h tools/dcas_node.h
	g++ -m32 -I../common x86Test.cpp -o dcas -lpthread

clean:
	rm -f *.o perf correctness dcas
