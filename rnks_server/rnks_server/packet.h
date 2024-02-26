#define BUFFER_SIZE 1024

typedef struct packet {
	char txtCol[BUFFER_SIZE];
	int seqNr;
	long checkSum;
};