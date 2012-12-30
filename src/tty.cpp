#include "tty.hpp"
#include "session.hpp"

void TTY::set_session(PosixSession& session) {
  pgrp_ = session.pgrp();
  session_ = session.session();
}
