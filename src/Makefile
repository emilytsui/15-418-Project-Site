all: myht correctness

myht: perfTest.cpp fg_hash_table.h seq_hash_table.h linked_list_node.h
	g++ -I../common perfTest.cpp -o myht -lpthread

correctness: correctnessTest.cpp fg_hash_table.h seq_hash_table.h linked_list_node.h
	g++ -I../common correctnessTest.cpp -o correctness -lpthread

clean:
	rm -f *.o myht correctness
