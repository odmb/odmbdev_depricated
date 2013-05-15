#include "emu/odmbdev/LogAction.h"

namespace emu { namespace odmbdev {
    LogAction::LogAction(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */}
    void LogAction::respond(xgi::Input * in, ostringstream & out) {
      XCEPT_RAISE( xcept::Exception, "Don't use LogActions as Actions." );
    }
  }
}
