#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <zlib.h>

struct flat_hdr {
	char magic[4];
	unsigned long rev;
	unsigned long entry;
	unsigned long data_start;
	unsigned long data_end;
	unsigned long bss_end;
	unsigned long stack_size;
	unsigned long reloc_start;
	unsigned long reloc_count;
	unsigned long flags;
	unsigned long filler[6];
};

static z_stream stream;

static int decompress(char *buf, int len, char *outbuf, int outbuf_len)
{
	int err;

	stream.next_in = buf;
	stream.avail_in = len;

	stream.next_out = outbuf;
	stream.avail_out = outbuf_len;

	inflateReset(&stream);

	err = inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		fprintf(stderr, "decompression error %p(%d): %s\n",
			buf, len, zError(err));
		return -1;
	}
	return stream.total_out;
}


#define MAXSIZE (1024*1024)
int main(int argc, char **argv)
{
	int infd, outfd, offset, endoffset, num;
	char *inbuf, *outbuf;
	struct stat st;

	infd = open(argv[1], O_RDONLY, 0);
	if (infd < 0) {
		fprintf(stderr, "unable to open infile\n");
		exit(2);
	}

	outfd = open("out", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if (outfd < 0) {
		fprintf(stderr, "unable to open outfile\n");
		exit(2);
	}

	fstat(infd, &st);
	inbuf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, infd, 0);
	if (inbuf == MAP_FAILED) {
		fprintf(stderr, "unable to mmap infile\n");
		exit(1);
	}

	ftruncate(outfd, MAXSIZE);
	outbuf = mmap(NULL, MAXSIZE, PROT_WRITE, MAP_SHARED, outfd, 0);
	if (outbuf == MAP_FAILED) {
		fprintf(stderr, "unable to mmap outfile\n");
		exit(1);
	}

	if (memcmp(inbuf, "bFLT", 4)) {
		fprintf(stderr, "not a bFLT file\n");
		exit(1);
	}

	offset = *((u_int32_t *)inbuf +3);
	endoffset = *((u_int32_t *)inbuf +4);
	fprintf(stderr, "offset=0x%lx\n", offset);
	
	inflateInit(&stream);
	num = decompress(inbuf+offset, st.st_size-offset, outbuf, MAXSIZE);
	inflateEnd(&stream);
	ftruncate(outfd, num);

	if (num == MAXSIZE)
		fprintf(stderr, "buffer too small\n");

	munmap(outbuf, MAXSIZE);
	munmap(inbuf, st.st_size);
	close(outfd);
	close(infd);
}
