#ifndef FS_EXT2_HPP
#define FS_EXT2_HPP

#include "common.hpp"
#include "block.hpp"
#include "fs.hpp"

namespace ext2 {

  enum SpecialInodes {
    eBadInode = 1,
    eRootInode = 2,
    eACLIndexInode = 3,
    eACLDataInode = 4,
    eBootInode = 5,
    eUndeleteInode = 6
  };

  const static u16 cMagic = 0xEF53;
  const static int cLinkMax = 32000;

  struct ACLHeader {
    u32 size;
    u32 file_count;
    u32 acle_count;
    u32 first_acle;
  };

  struct ACLEntry {
    u32 size;
    u16 perms;
    u16 type;
    u16 tag;
    u16 pad1;
    u32 next;
  };

  struct GroupDesc {
    u32 block_bitmap;
    u32 inode_bitmap;
    u32 inode_table;
    u16 free_blocks_count;
    u16 free_inodes_count;
    u16 used_dirs_count;
    u16 pad;
    u32 reserved[3];
  };

  const static u32 NumDirectBlocks = 12;

  struct Inode {
    u16 mode;
    u16 uid;
    u32 size;
    u32 atime;
    u32 ctime;
    u32 mtime;
    u32 dtime;
    u16 gid;
    u16 link_count;
    u32 block_count;
    u32 flags;
    u32 osd1;
    u32 blocks[NumDirectBlocks];
    u32 indirect_block;
    u32 double_indirect_block;
    u32 triple_indirect_block;
    u32 version;
    u32 file_acl;
    u32 dir_acl;
    u32 osd2_1;
    u32 osd2_2;
    u32 osd2_3;

    enum Flags {
      eSecureDelete = 0x1,
      eUndelete = 0x2,
      eCompress = 0x4,
      eSync = 0x8,
      eImmutable = 0x10,
      eAppendOnly = 0x20,
      eNoDump = 0x40,
      eNoAtime = 0x80,
      eDirty = 0x100,
      eCompressedBlock = 0x200,
      eNoCompression = 0x400,
      eCompressionError = 0x800,
      eBTree = 0x1000,
      eHashIndex = 0x1000,
      eIMagic = 0x2000,
      eJournalData = 0x4000,
      eNoTail = 0x8000,
      eDirSync = 0x10000,
      eTopDir =  0x20000,
      eExtents = 0x80000,
      eDirectIO = 0x100000,
      eReserved = 0x80000000
    };

    enum Modes {
      eIFSocket = 0xc000,
      eIFLink = 0xa000,
      eIFReg = 0x8000,
      eIFBlock = 0x6000,
      eIFDir = 0x4000,
      eIFChar = 0x2000,
      eIFFifo = 0x1000,

      eIFMask = 0xf000,

      eSetUID = 0x800,
      eSetGID = 0x400,
      eSticky = 0x200,

      eUserRead = 0x100,
      eUserWrite = 0x80,
      eUserExec = 0x40,
      eGroupRead = 0x20,
      eGroupWrite = 0x10,
      eGroupExec = 0x8,
      eOtherRead = 0x4,
      eOtherWrite = 0x2,
      eOtherExec = 0x1
    };

    int type() {
      return mode & eIFMask;
    }

    bool socket_p() {
      return (type() & eIFSocket) == eIFSocket;
    }

    bool symlink_p() {
      return (type() & eIFLink) == eIFLink;
    }

    bool file_p() {
      return (type() & eIFReg) == eIFReg;
    }

    bool blockdev_p() {
      return (type() & eIFBlock) == eIFBlock;
    }

    bool directory_p() {
      return (type() & eIFDir) == eIFDir;
    }

    bool chardev_p() {
      return (type() & eIFChar) == eIFChar;
    }

    bool fifo_p() {
      return (type() & eIFFifo) == eIFFifo;
    }

    void print_info();
  };

  struct SuperBlock {
    u32 inodes_count;
    u32 blocks_count;
    u32 reserved_blocks_count;
    u32 free_blocks_count;
    u32 free_inodes_count;
    u32 first_data_block;
    u32 log_block_size;
    u32 log_frag_size;
    u32 blocks_per_group;
    u32 flags_per_group;
    u32 inodes_per_group;
    u32 mtime;
    u32 wtime;
    u16 mount_count;
    s16 max_mount_count;
    u16 magic;
    u16 state;
    u16 errors;
    u16 minor_rev_level;
    u32 lastcheck;
    u32 check_interval;
    u32 creator_os;
    u32 rev_level;
    u16 default_res_uid;
    u16 default_res_gid;

    u32 first_inode;
    u16 inode_size;
    u16 block_group_number;
    u32 feature_compat;
    u32 feature_required;
    u32 feature_ro_compat;

    u8  uuid[16];
    char volume_name[16];
    char last_mounted[64];
    u32 algo_usage_bitmap;
    u8  prealloc_blocks;
    u8  prealloc_dir_blocks;
    u16 padding1;

    u8  journal_uuid[16];
    u32 journal_inum;
    u32 journal_dev;
    u32 last_orphan;
    u32 hash_seed[4];
    u8  def_hash_version;
    u8  padding2;
    u16 padding3;
    u32 default_mount_opts;
    u32 first_meta_group;
    u32 reserved[190];

    enum States {
      eValidFS = 0x1,
      eErrorFS = 0x2
    };

    enum Flags {
      eMountCheck = 0x1,
      eMountOldAlloc = 0x2,
      eMountGroup = 0x4,
      eMountDebug = 0x8,
      eMountErrorContinue = 0x10,
      eMountErrorReadOnly = 0x20,
      eMountErrorPanic = 0x40,
      eMountMinixStyle = 0x80,
      eMountNoBuffers = 0x100,
      eMountNoUID32 = 0x200,
      eMountXAttr = 0x4000,
      eMountPosixACL = 0x8000,
      eMountXIP = 0x10000,
      eMountUserQuota = 0x20000,
      eMountGroupQuota = 0x40000,
      eMountReservation = 0x80000,


      eErrorContinue = 1,
      eErrorReadOnly = 2,
      eErrorPanic = 3,
      eErrorDefault = eErrorContinue
    };

  public:
    bool validate();
    void print_features();
    GroupDesc* read_block_groups(block::Device* dev, u32* total);
  };

  enum CreatorOS {
    eLinux = 0,
    eHurd = 1,
    eMasix = 2,
    eFreeBSD = 3,
    eLites = 4,
    eUlysses = 47
  };


  enum Revisions {
    eOriginalRev = 0,
    eDynamicRev = 1,
    eCurrentRev = 1,
    eMaxRev = 1
  };

  enum Features {
    eFeatureDirPrealloc = 0x1,
    eFeatureImagicInodes = 0x2,
    eFeatureHasJournal = 0x4,
    eFeatureXAttr = 0x8,
    eFeatureResizeInode = 0x10,
    eFeatureDirIndex = 0x20,

    eROFeatureSparseSuper = 0x1,
    eROFeatureLargeFile = 0x2,
    eROBTreeDir = 0x4,

    eReqFeatureCompression = 0x1,
    eReqFeatureFileType = 0x2,
    eReqFeatureRecover = 0x4,
    eReqFeatureJournalDev = 0x8,
    eReqFeatureMetaBlock = 0x10
  };

  const static int cNameLength = 255;

  struct DirEntry0 {
    u32 inode;
    u16 rec_len;
    u16 name_len;
    char name[cNameLength];
  };

  struct DirEntry {
    u32 inode;
    u16 rec_len;
    u8 name_len;
    u8 file_type;
    char name[cNameLength];

    enum FileTypes {
      eUnknown = 0,
      eRegular = 1,
      eDirector = 2,
      eCharDevice = 3,
      eBlockDevice = 4,
      eFIFO = 5,
      eSocket = 6,
      eSymLink = 7,
      eMax = 8
    };
  };

  struct Node;

  class FS {
    block::Device* device_;

    SuperBlock* super_block_;
    u32 total_groups_;
    GroupDesc* groups_;

  public:
    block::Device* device() {
      return device_;
    }

    FS(block::Device* dev);

    SuperBlock* read_superblock();

    Node* root();
    void print_description();
    Inode* find_inode(u32 inode);
  };

  struct Node : public fs::Node {
    FS* fs_;

    virtual u32 read(u32 offset, u32 size, u8* buffer);
    virtual u32 write(u32 offset, u32 size, u8* buffer);
    virtual void open();
    virtual void close();
    virtual struct dirent* readdir(u32 index);
    virtual Node* finddir(const char* name, int len);
  };

  class RegisteredFS : public fs::RegisteredFS {
  public:
    RegisteredFS(const char* name)
      : fs::RegisteredFS(name)
    {}

    fs::Node* load(block::Device* dev);
  };

  void init();
}

#endif
