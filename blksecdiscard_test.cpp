#include <iostream>

#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>

#include <linux/fs.h>
#include <string.h>

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>


using std::string;
using std::cout;
using std::endl;

void help(const char *cmd, string str)
{
	cout << str << endl;
	cout << "usage: "
		<< cmd << " <blk_device> <secure | nonsecure>\n\n"
		<< "  <blk_device>: the block device to be discarded\n"
		<< "  <secure | nonsecure>: choose between the BLKSECDISCARD and the BLKDISCARD command\n"
		<< endl;
}

unsigned long long get_block_device_size(int fd)
{
	unsigned long long size = 0;
	int ret;

	ret = ioctl(fd, BLKGETSIZE64, &size);

	if (ret) return 0;

	return size;

}

int wipe_block_device(int fd, unsigned long long length, bool secure)
{
	unsigned long long range[2];
	int ret;
	int req;

	range[0] = 0;
	range[1] = length;
	if (secure) {
		req = BLKSECDISCARD;
	} else {
		req = BLKDISCARD;
	}

	ret = ioctl(fd, req, &range);
	if (ret < 0) {
		cout << (secure? "Secure" : "Nonsecure") << " discard failed: " << strerror(errno) << endl;
	}

	return ret;
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		help(argv[0], string("not enough args"));
		return 1;
	}

	// check whether the first argument is a block device that can be accessed
	if (access(argv[1], F_OK)) {
		help(argv[0], string("the 1st argument \'") + argv[1] + "\' cannot be accessed");
		return 1;
	}

	struct stat st;
	int ret = stat(argv[1], &st);
	if (ret < 0 ) {
		help(argv[0], string("fail to stat on \'") + argv[1] + "\'");
		return 1;
	}
	if (!S_ISBLK(st.st_mode)) {
		help(argv[0], argv[1] + string(" is not a block device"));
		return 1;
	}

	bool secure = true;
	if (!strncmp("nonsecure", argv[2], 9)) {
		secure = false;
	} else {
		if (strncmp("secure", argv[2], 6))
			cout << "ioctl BLKSECDISCARD command to be execute because the 2nd arg is not \'nonsecure\'" << endl;
	}

	// open the block device
	int fd = open(argv[1], O_RDWR);
	if(fd < 0) {
		cout << "fail to open the device " << argv[1] << " " << strerror(errno) << endl;
		return 1;
	}

	unsigned long long device_size = get_block_device_size(fd);
	if (0 == device_size) {
		cout << "fail to get the block device size" << endl;
		return 1;
	}
	cout << "the block device size is: " << device_size << endl;

	wipe_block_device(fd, device_size, secure);

	close(fd);

	return 0;
}
