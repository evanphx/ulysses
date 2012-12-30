#ifndef TTY_HPP
#define TTY_HPP

class PosixSession;

class TTY {
  int pgrp_;
  int session_;

  bool canonical_;
  bool echo_;

public:

  TTY()
    : pgrp_(0)
    , session_(0)
    , canonical_(true)
    , echo_(true)
  {}

  bool canonical_p() {
    return canonical_;
  }

  void set_canonical(bool val) {
    canonical_ = val;
  }

  bool echo_p() {
    return echo_;
  }

  void set_echo(bool val) {
    echo_ = val;
  }

  void set_session(PosixSession& session);
};

#endif
