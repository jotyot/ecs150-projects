#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <stack>
#include <vector>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

string getFullPath(LocalFileSystem *fs, int inodeNumber);
string getNameOfInode(LocalFileSystem *fs, int inodeNumber);
int getParentInodeNum(LocalFileSystem *fs, int inodeNumber);
inode_t getInode(LocalFileSystem *fs, int inodeNumber);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fs = new LocalFileSystem(disk);

  stack<int> inodeStack;
  inodeStack.push(UFS_ROOT_DIRECTORY_INODE_NUMBER);

  // need to somehow store the current path of the directory we are in
  // the current inode being processed would be the end of the path
  // hmm i guess i can recursively get the parent inode of the current inode

  while (!inodeStack.empty()) {
    int inodeNumber = inodeStack.top();
    inodeStack.pop();

    inode_t inode;
    fs->stat(inodeNumber, &inode);

    if (inode.type == UFS_DIRECTORY) {
      dir_ent_t entries[inode.size / sizeof(dir_ent_t)];
      fs->read(inodeNumber, &entries, inode.size);

      sort(entries + 2, entries + (inode.size / sizeof(dir_ent_t)), [](dir_ent_t a, dir_ent_t b) {
        return strcmp(a.name, b.name) > 0;
      });

      for (dir_ent_t entry : entries) {
        inode_t entryInode;
        fs->stat(entry.inum, &entryInode);
        if (entryInode.type == UFS_DIRECTORY && strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0 ) {  
          inodeStack.push(entry.inum);
        }
      }

      sort(entries, entries + (inode.size / sizeof(dir_ent_t)), [](dir_ent_t a, dir_ent_t b) {
        return strcmp(a.name, b.name) < 0;
      });

      cout << "Directory " << getFullPath(fs, inodeNumber) << endl;
      for (dir_ent_t entry : entries) {
        cout << entry.inum << '\t' << entry.name << endl;
      }
      cout << endl;
    }
  }

  return 0;
}

string getFullPath(LocalFileSystem *fs, int inodeNumber) {
  inode_t inode;
  fs->stat(inodeNumber, &inode);
  if (inodeNumber == UFS_ROOT_DIRECTORY_INODE_NUMBER) {
    return "/";
  }
  return getFullPath(fs, getParentInodeNum(fs, inodeNumber)) + getNameOfInode(fs, inodeNumber) + "/";
}

// it feels like the only way to get the name of the inode is to read the parent inode
// you can get the parent inode with direct[0] of the inode
// then read the parent inode and look for the entry with the inode number of the inode
string getNameOfInode(LocalFileSystem *fs, int inodeNumber) {
  int parentInodeNumber = getParentInodeNum(fs, inodeNumber);
  inode_t parent_inode = getInode(fs, parentInodeNumber);

  dir_ent_t parent_entries[parent_inode.size / sizeof(dir_ent_t)];
  fs->read(parentInodeNumber, &parent_entries, parent_inode.size);

  for (dir_ent_t entry : parent_entries) {
    if (entry.inum == inodeNumber) {
      return entry.name;
    }
  }

  return "";
}

int getParentInodeNum(LocalFileSystem *fs, int inodeNumber) {
  inode_t inode;
  fs->stat(inodeNumber, &inode);
  dir_ent_t entries[inode.size / sizeof(dir_ent_t)];
  fs->read(inodeNumber, &entries, inode.size);

  return entries[1].inum;
}

inode_t getInode(LocalFileSystem *fs, int inodeNumber) {
  inode_t inode;
  fs->stat(inodeNumber, &inode);
  return inode;
}
