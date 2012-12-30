#ifndef POSIX_SESSION_HPP
#define POSIX_SESSION_HPP

#include "tty.hpp"

class PosixSession {
  bool leader_;
  int pgrp_;
  int session_;

  int uid_, euid_, suid_;
  int gid_, egid_, sgid_;

  TTY* tty_;

public:

  PosixSession(PosixSession& parent)
    : leader_(false)
    , pgrp_(parent.pgrp_)
    , session_(parent.session_)
    , uid_(parent.uid_), euid_(parent.euid_), suid_(parent.suid_)
    , gid_(parent.gid_), egid_(parent.egid_), sgid_(parent.sgid_)
    , tty_(parent.tty_)
  {}

  enum Special { Init };

  PosixSession(enum Special init)
    : leader_(true)
    , pgrp_(0)
    , session_(0)
    , uid_(0), euid_(0), suid_(0)
    , gid_(0), egid_(0), sgid_(0)
    , tty_(0)
  {}

  int pgrp() {
    return pgrp_;
  }

  int session() {
    return session_;
  }

  int uid() {
    return uid_;
  }

  int euid() {
    return euid_;
  }

  int suid() {
    return suid_;
  }

  int gid() {
    return gid_;
  }

  int egid() {
    return egid_;
  }

  int sgid() {
    return sgid_;
  }

  TTY* tty() {
    return tty_;
  }

  void set_tty(TTY* tty) {
    if(leader_ && !tty_) {
      tty_ = tty;
      tty->set_session(*this);
    }
  }

};

#endif
