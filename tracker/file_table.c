

// create a new file table
// ret: (not claimed) newly allocated table
ChunkyFileTable* filetable_createTable(){
	ChunkyFileTable* table = (ChunkyFileTable*)malloc(sizeof(ChunkyFileTable));
	table->numberOfFiles = 0;
	table->head = (ChunkyFileTable_file*)malloc(sizeof(ChunkyFileTable_file));
	table->tail = (ChunkyFileTable_file*)malloc(sizeof(ChunkyFileTable_file));
	table->tail = NULL;
	table->head = table->tail;
	return table;
}

// create a new file table
// table: (not claimed) table to destroy
// ret: (static) 1 if success, -1 if failure
int filetable_destroyTable(ChunkyFileTable* table){
	ChunkyFileTable_file* iterator;
	ChunkyFileTable_file* temp;
	iterator = table->head;
	while (iterator->next != table->tail){
		temp = iterator;
		iterator = temp->next;
		filetable_destroyFile(temp);
	}
	free(table);
	return 1;
}


// // saves a new file that has been created to the table
// //	path: (not claimed) a file path to use as a descriptor/identifier for the new file
// //	size: (not claimed) the size of the file
// //	numChunks: (not claimed) the number of chunks needed for the file (THIS MAY NOT BE NECESSARY, can calculate in add)
// //	peerID: (not claimed) the peerID that made the file. should be added as owning all chunks
// // 	ret: (static) 1 on success, -1 on failure
// int filetable_addFile(char* path, int size, int numChunks, int peerID, ChunkyFileTable* table){
// 	ChunkyFileTable_file* file = (ChunkyFileTable_file*)malloc(sizeof(ChunkyFileTable_file));
// 	file->name = path;
// 	file->size = size;
// 	file->numChunks = numChunks;
// 	if (table->head == table->tail){
// 		table->head = 
// 	}


// }

// ChunkyFileTable_file* filetable_createFile(char* path, int size, int numChunks, int peerID){
// 	ChunkyFileTable_file* file = (ChunkyFileTable_file*)malloc(sizeof(ChunkyFileTable_file));
// 	file->name = path;
// 	file->size = size;
// 	file->numChunks = numChunks;
// 	file->next = NULL;
// 	// create chunks
// 	ChunkyFileTable_chunk* firstChunk = (ChunkyFileTable_chunk*)malloc(sizeof(ChunkyFileTable_chunk);
// 	firstChunk->chunkNum = 1;
// 	firstChunk->numberOfPeers = 1;
// 	firstChunk->peerID = (int*)malloc(firstChunk->numberOfPeers*sizeof(int));
// 	firstChunk->peerID[0] = peerID;
// 	file->head = firstChunk;
// 	if (numChunks == 1){
// 		file->tail = firstChunk;
// 		file->next = NULL;
// 	}else{
// 		while()
// 	}
// }


