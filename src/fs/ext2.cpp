#include "fs/ext2.hpp"
#include "block.hpp"
#include "console.hpp"

namespace ext2 {

  void init() {
    RegisteredFS* fs = new(kheap) RegisteredFS("ext2");
    fs::registry.add_fs(fs);
  }

  fs::Node* RegisteredFS::load(block::Device* dev) {
    if(!dev) return 0;

    FS* fs = new(kheap) FS(dev);
    if(fs->read_superblock()) {
      fs->print_description();
      return fs->root();
    }

    return 0;
  }

  FS::FS(block::Device* dev)
    : device_(dev)
  {}

  void FS::print_description() {
    super_block_->print_features();
  }

  bool SuperBlock::validate() {
    return magic == cMagic;
  }

  void SuperBlock::print_features() {
    console.printf("Optional features: ");

    if(feature_compat & eFeatureDirPrealloc) {
      console.printf("dir_prealloc ");
    }

    if(feature_compat & eFeatureImagicInodes) {
      console.printf("imagic_inodes ");
    }

    if(feature_compat & eFeatureXAttr) {
      console.printf("xattr ");
    }

    if(feature_compat & eFeatureResizeInode) {
      console.printf("resize_inode ");
    }

    if(feature_compat & eFeatureDirIndex) {
      console.printf("dir_index ");
    }

    console.printf("\n");

    console.printf("Required features: ");

    if(feature_ro_compat & eROFeatureSparseSuper) {
      console.printf("sparse_super(ro) ");
    }

    if(feature_ro_compat & eROFeatureLargeFile) {
      console.printf("large_file(ro) ");
    }

    if(feature_ro_compat & eROBTreeDir) {
      console.printf("btree_dir(ro) ");
    }

    if(feature_required & eReqFeatureCompression) {
      console.printf("compression ");
    }

    if(feature_required & eReqFeatureFileType) {
      console.printf("file_type ");
    }

    if(feature_required & eReqFeatureRecover) {
      console.printf("recover ");
    }

    if(feature_required & eReqFeatureJournalDev) {
      console.printf("journal_dev ");
    }

    if(feature_required & eReqFeatureMetaBlock) {
      console.printf("meta_block ");
    }

    console.printf("\n");
  }

  GroupDesc* SuperBlock::read_block_groups(block::Device* dev, u32* total) {
    int group_count = (blocks_count - first_data_block +
                       blocks_per_group - 1) / blocks_per_group;

    console.printf("Group count: %d\n", group_count);

    u32 table_size = sizeof(GroupDesc) * group_count;
    GroupDesc* groups = (GroupDesc*)kmalloc(table_size);

    *total = group_count;

    dev->read_bytes(2048, table_size, (u8*)groups);
    return groups;
  }

  SuperBlock* FS::read_superblock() {
    SuperBlock* sb = new(kheap) SuperBlock;

    device_->read_bytes(1024, sizeof(SuperBlock), (u8*)sb);

    if(!sb->validate()) return 0;

    super_block_ = sb;

    groups_ = sb->read_block_groups(device_, &total_groups_);
    return sb;
  }

  /*
  void FS::init() {
    head_ = 0;

    root_ = new(kheap) Node;
    root_->name[0] = '/';
    root_->name[1] = 0;

    root_->mask = 0;
    root_->uid = 0;
    root_->gid = 0;
    root_->flags = FS_DIRECTORY;
    root_->block_dev = 0;
    root_->inode = 0;
    root_->length = 0;
    root_->delegate = 0;
    root_->next = 0;
  }
  */

  Node* FS::root() {
    Node* node = new(kheap) Node;

    node->name[0] = '/';
    node->name[1] = 0;

    node->mask = 0;
    node->uid = 0;
    node->gid = 0;
    node->flags = FS_DIRECTORY;
    node->inode = eRootInode;
    node->length = 0;
    node->delegate = 0;
    node->fs_ = this;

    return node;
  }

  u32 Node::read(u32 offset, u32 size, u8* buffer) {
    console.printf("reading from inode %d, offset=%d\n", inode, offset);

    Inode* obj = fs_->find_inode(inode);

    u32 cur_block = offset / 1024;
    u32 initial_offset = offset % 1024;

    ASSERT(cur_block + (size / 1024) < 12);

    u32 block1 = obj->blocks[cur_block];

    u32 b1_size = 1024 - initial_offset;

    if(b1_size > size) b1_size = size;

    fs_->device()->read_bytes((block1 * 1024) + initial_offset,
                              b1_size, buffer);
    u32 left = size - b1_size;

    if(left == 0) return size;

    buffer += b1_size;

    cur_block++;

    for(; cur_block < NumDirectBlocks; cur_block++) {
      u32 read_size = (left > 1024) ? 1024 : left;

      fs_->device()->read_bytes(obj->blocks[cur_block] * 1024,
                                read_size, buffer);

      buffer += read_size;
      left -= read_size;
      if(left == 0) return size;
    }

    // Don't get here.
    ASSERT(0);
    return -1;
  }

  void Inode::print_info() {
    console.printf("type: ");

    int m = type();

    console.printf("(%d) ", m);

    if(socket_p()) {
      console.printf("socket ");
    }

    if(symlink_p()) {
      console.printf("symlink ");
    }

    if(file_p()) {
      console.printf("regular ");
    }

    if(blockdev_p()) {
      console.printf("block ");
    }

    if(directory_p()) {
      console.printf("directory ");
    }

    if(chardev_p()) {
      console.printf("chardev ");
    }

    if(fifo_p()) {
      console.printf("fifo ");
    }

    console.printf("\n");
  }

  Inode* FS::find_inode(u32 inode) {
    u32 bg  = (inode - 1) / super_block_->inodes_per_group;
    ASSERT(bg < total_groups_);

    GroupDesc* group = &groups_[bg];

    u32 group_offset = (inode - 1) % super_block_->inodes_per_group;

    u32 entry_offset = (group->inode_table * 1024) +
                       (group_offset * super_block_->inode_size);

    Inode* inode_obj = (Inode*)kmalloc(super_block_->inode_size);

    console.printf("inode %d is @ [%d,%d], byte=%d\n",
                   inode, bg, group_offset, entry_offset);

    device_->read_bytes(entry_offset, super_block_->inode_size,
                        (u8*)inode_obj);

    console.printf("Got inode: %d\n", inode_obj->mode);
    inode_obj->print_info();

    return inode_obj;
  }

  Node* Node::finddir(const char* name, int len) {
    console.printf("look for '%s' (%d) in ext2\n", name, len);
    Inode* obj = fs_->find_inode(inode);

    if(!obj->directory_p()) return 0;

    if(obj->block_count == 0) return 0;

    u32 block1 = obj->blocks[0];

    u8* entries = (u8*)kmalloc(1024);

    fs_->device()->read_bytes(block1 * 1024, 1024, (u8*)entries);

    u8* fin = entries + 1024;

    // char buf[256];

    while(entries < fin) {
      DirEntry* dir = (DirEntry*)entries;

      // memcpy((u8*)buf, (u8*)dir->name, dir->name_len);
      // buf[dir->name_len] = 0;

      // console.printf("dir entry: '%s' inode=%d, rec_len=%d, type=%d\n", buf,
                      // dir->inode, dir->rec_len, dir->file_type);

      if(!strncmp(dir->name, name, len)) {
        Inode* target = fs_->find_inode(dir->inode);

        Node* node = new(kheap) Node;

        memcpy((u8*)node->name, (u8*)dir->name, dir->name_len);
        node->name[dir->name_len] = 0;

        node->mask = 0;
        node->uid = target->uid;
        node->gid = target->gid;

        if(target->directory_p()) {
          node->flags = FS_DIRECTORY;
        } else {
          node->flags = FS_FILE;
        }

        node->inode = dir->inode;
        node->length = target->size;

        node->delegate = 0;
        node->fs_ = fs_;

        return node;
      }

      entries += dir->rec_len;
    }


    return 0;
    /*
    Node* node = main.head();

    while(node) {
      if(!strncmp(node->name, name, len)) return node;
      node = node->next;
    }

    return 0;
    */
  }
  
  u32 Node::write(u32 offset, u32 size, u8* buffer) { return 0; }
  void Node::open() { return; }
  void Node::close() { return; }
  struct dirent* Node::readdir(u32 index) { return 0; }
}
