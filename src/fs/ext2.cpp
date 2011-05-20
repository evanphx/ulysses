#include "fs/ext2.hpp"
#include "block.hpp"
#include "console.hpp"
#include "cpu.hpp"

#include "block_region.hpp"
#include "block_buffer.hpp"

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

  FS::FS(block::Device* dev)
    : device_(dev)
  {
    // Sensible defaults, changed when the superblock is read.
    block_per_region_ = 1;
    bytes_per_block_ = 1024;
    ids_per_block_ = 256;
    ids_per_second_level_ = ids_per_block_ * ids_per_block_;
  }

  void FS::print_description() {
    super_block_->print_features();
  }

  block::Buffer* FS::async_read_block(u32 block, u32 count) {
    block::RegionRange range(b2r(block), b2r(count));
    block::Buffer* buffer = device_->request(range);

    return buffer;
  }

  block::Buffer* FS::read_block(u32 block, u32 count) {
    block::RegionRange range(b2r(block), b2r(count));
    block::Buffer* buffer = device_->request(range);

    buffer->wait();

    return buffer;
  }

  block::Buffer* FS::read_inode_block(u32 inode, Inode* obj, u32 block) {
    block::Buffer* buffer;
    u32 id;

    auto key = sys::pair(inode, block);

    if(block_cache_.fetch(key, &buffer)) {
      return buffer;
    }

    // Read the block id directly.
    if(block < NumDirectBlocks) {
      buffer = read_block(obj->blocks[block]);
      block_cache_.store(key, buffer);
      return buffer;
    }

    block -= NumDirectBlocks;

    // Read via a single indirection table
    if(block < ids_per_block_) {
      buffer = read_block(obj->blocks[eIndirectBlock]);

      id = buffer->index<u32>(block);
      if(!id) return 0;

      buffer = read_block(id);
      block_cache_.store(key, buffer);
      return buffer;
    }

    block -= ids_per_block_;

    // Read via 2 indirection tables
    if(block < ids_per_second_level_) {
      buffer = read_block(obj->blocks[eDoubleIndirectBlock]);

      id = buffer->index<u32>(block / ids_per_block_);
      if(!id) return 0;

      buffer = read_block(id);

      id = buffer->index<u32>(block % ids_per_block_);
      if(!id) return 0;

      buffer = read_block(id);
      block_cache_.store(key, buffer);
      return buffer;
    }

    // Read via 3 indirection tables
    block -= ids_per_second_level_;
    buffer = read_block(obj->blocks[eTripleIndirectBlock]);

    id = buffer->index<u32>(block / ids_per_second_level_);
    if(!id) return 0;

    buffer = read_block(id);

    id = buffer->index<u32>(block / ids_per_block_);
    if(!id) return 0;

    buffer = read_block(id);

    id = buffer->index<u32>(block % ids_per_block_);
    if(!id) return 0;

    buffer = read_block(id);
    block_cache_.store(key, buffer);
    return buffer;
  }

  GroupDesc* FS::read_block_groups() {
    total_groups_ = (super_block_->blocks_count -
                     super_block_->first_data_block +
                     super_block_->blocks_per_group - 1)
                  / super_block_->blocks_per_group;

    u32 table_size = sizeof(GroupDesc) * total_groups_;
    groups_ = (GroupDesc*)kmalloc(table_size);

    block::Buffer* buffer = read_block(2, total_groups_);

    memcpy((u8*)groups_, buffer->data(), table_size);
    return groups_;
  }

  SuperBlock* FS::read_superblock() {
    SuperBlock* sb = new(kheap) SuperBlock;

    block::RegionRange range(block::byte2region(1024),
                             block::byte2region(1024));

    block::Buffer* buffer = device_->request(range);

    buffer->wait();

    memcpy((u8*)sb, buffer->data(), sizeof(SuperBlock));

    if(!sb->validate()) return 0;

    super_block_ = sb;

    bytes_per_block_ = 1024 << sb->log_block_size;

    ASSERT(bytes_per_block_ >= block::cRegionSize);

    block_per_region_ = bytes_per_block_ / block::cRegionSize;
    inode_per_block_ = bytes_per_block_ / sb->inode_size;

    ids_per_block_ = bytes_per_block_ / sizeof(u32);
    ids_per_second_level_ = ids_per_block_ * ids_per_block_;

    read_block_groups();
    return sb;
  }

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

  u32 Node::read(u32 offset, u32 size, u8* user) {
    Inode* obj = fs_->find_inode(inode);

    u32 bpb = fs_->bytes_per_block();

    u32 cur_block = offset / bpb;
    u32 initial_offset = offset % bpb;

    u32 b1_size = bpb - initial_offset;

    if(b1_size > size) b1_size = size;

    block::Buffer* buffer = fs_->read_inode_block(inode, obj, cur_block++);

    memcpy((u8*)user, buffer->data() + initial_offset, b1_size);

    u32 left = size - b1_size;

    if(left == 0) return size;

    user += b1_size;

    while(left > 0) {
      u32 read_size = (left > bpb) ? bpb : left;

      buffer = fs_->read_inode_block(inode, obj, cur_block++);

      memcpy((u8*)user, buffer->data(), read_size);

      user += read_size;
      left -= read_size;
    }

    return size;
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
    u32 adj_inode = inode - 1;
    u32 bg  = adj_inode / super_block_->inodes_per_group;
    ASSERT(bg < total_groups_);

    GroupDesc* group = &groups_[bg];

    u32 block = group->inode_table + (adj_inode / inode_per_block());
    u32 block_offset = adj_inode % inode_per_block();

    block::Buffer* buffer = read_block(block);

    Inode* inode_obj = (Inode*)kmalloc(super_block_->inode_size);

    memcpy((u8*)inode_obj,
           buffer->data() + (block_offset * super_block_->inode_size),
           super_block_->inode_size);

    return inode_obj;
  }

  Node* Node::finddir(const char* name, int len) {
    Inode* obj = fs_->find_inode(inode);

    if(!obj->directory_p()) {
      console.printf("inode %d not a directory.\n", inode);
      return 0;
    }

    if(obj->block_count == 0) return 0;

    u32 block1 = obj->blocks[0];

    block::Buffer* buffer = fs_->read_block(block1);

    u8* entries = buffer->data();

    u8* fin = entries + fs_->bytes_per_block();

    while(entries < fin) {
      DirEntry* dir = (DirEntry*)entries;

      if(!strncmp(dir->name, name, len)) {
        Node* node = fs_->find_in_use(dir->inode);
        if(node) return node;

        Inode* target = fs_->find_inode(dir->inode);

        node = new(kheap) Node;

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

        fs_->make_in_use(dir->inode, node);

        return node;
      }

      entries += dir->rec_len;
    }

    return 0;
  }
  
  u32 Node::write(u32 offset, u32 size, u8* buffer) { return 0; }
  void Node::open() { return; }
  void Node::close() { return; }
  struct dirent* Node::readdir(u32 index) { return 0; }
}
