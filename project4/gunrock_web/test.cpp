#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <fstream>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fs = new LocalFileSystem(disk);

  // read contents of test.txt into a string
  ifstream testFile("test.txt");
  string testContents((istreambuf_iterator<char>(testFile)), istreambuf_iterator<char>());

  int newInum = fs->create(UFS_ROOT_DIRECTORY_INODE_NUMBER, UFS_DIRECTORY, "a");

  int bInum = fs->create(newInum, UFS_DIRECTORY, "b");

  int fileInode = fs->create(bInum, UFS_REGULAR_FILE, "c.txt");
  fs->create(bInum, UFS_REGULAR_FILE, "d.txt");

  fs->write(fileInode, testContents.c_str(), strlen(testContents.c_str()));

  char buffer[100];
  strcpy(buffer, "hello world\n");
  fs->write(fileInode, buffer, strlen(buffer));

  return 0;
}

