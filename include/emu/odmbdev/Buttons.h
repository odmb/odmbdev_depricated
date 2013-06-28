#ifndef __emu_odmbdev_Buttons_h__
#define __emu_odmbdev_Buttons_h__

#include "emu/pc/Crate.h" 
#include "emu/pc/VMEController.h"
#include "emu/pc/CFEB.h"
#include "emu/pc/DAQMB.h"
#include "emu/pc/CCB.h"
#include "emu/pc/DDU.h"
#include "emu/pc/TMB.h"
#include "emu/pc/TMB_constants.h"

#include "xgi/Method.h"

#include "xcept/tools.h"
#include "xcept/Exception.h"

#include <vector>
#include <sstream>
#include <iomanip>

#include "emu/odmbdev/Action.h"
#include "emu/odmbdev/ButtonAction.h"
#include "emu/odmbdev/LogAction.h"
#include "emu/odmbdev/LongTextBoxAction.h"
#include "emu/odmbdev/ThreeTextBoxAction.h"
#include "emu/odmbdev/utils.h"

/******************************************************************************
 * The Buttons
 *
 * This header file and the associated cc file contains all the actions in the
 * application. Short and simple ones are often defined in the header file so
 * as to avoid duplicating code (declaration and definition).
 *
 * If this file starts to take a very long time to compile, you ought to
 * consider breaking it into pieces (perhaps grouped by type of test [e.g. TMB,
 * DMB]). Just remember to include all the pieces in the Manager.h file.
 *
 * NB The Makefile will not check if this file has been modified, if you modify
 * it without modifying the Buttons.cc file, use the command `touch Buttons.cc'
 * to ensure that it is recompiled
 *****************************************************************************/

using namespace std;
using namespace emu::pc;

namespace emu { namespace odmbdev {

    /**************************************************************************
     * Utilities
     *
     * Simple functions
     *************************************************************************/

    string FixLength(unsigned int Number, int Length = 5, bool isHex = true);
    

    /**************************************************************************
     * Simple Buttons
     *
     * For tests/actions that are one or two lines and just need a button and
     * no text boxes 
     *************************************************************************/

    class HardReset : public ButtonAction {
    public:
      HardReset(Crate * crate) : ButtonAction(crate, "Hard Reset") { }
      void respond(xgi::Input * in, ostringstream & out) { ccb->hardReset(); }
    };

    class L1Reset : public ButtonAction {
    public:
      L1Reset(Crate * crate) : ButtonAction(crate, "L1 Reset (Resync)") { }
      void respond(xgi::Input * in, ostringstream & out) { ccb->l1aReset(); }
    };

    class BC0 : public ButtonAction {
    public:
      BC0(Crate * crate) : ButtonAction(crate, "BC0") { }
      void respond(xgi::Input * in, ostringstream & out) { ccb->bc0(); }
    };

    /**************************************************************************
     * Log Buttons
     *
     * These are special actions that modify the log
     *************************************************************************/

    class ClearLog : public LogAction {
    public:
      ClearLog(Crate * crate) : LogAction(crate) { }
      void display(xgi::Output * out) { AddButton(out, "Clear Log"); }
      void respond(xgi::Input * in, ostringstream & out, ostringstream & log) {
	log.clear(); // remove any error flags
	log.str(""); // empty the log
      }
    };

    /**************************************************************************
     * Other Buttons
     *
     *************************************************************************/

    class ReadBackUserCodes : public ButtonAction {
    public:
      ReadBackUserCodes(Crate * crate);
      void respond(xgi::Input * in, ostringstream & out);
    };

    /**************************************************************************
     * ExecuteVMEDSL
     *
     * A domain-specific-lanaguage for issuing vme commands.
     *************************************************************************/
    class ExecuteVMEDSL : public ThreeTextBoxAction {
    public:
      ExecuteVMEDSL(Crate * crate);
      void respond(xgi::Input * in, ostringstream & out);
      int nCommand;
    };
    /**************************************************************************
     * ResetRegisters
     *
     * A small class to implement a reset from the ODMB_CTRL bits
     **************************************************************************/
    class ResetRegisters : public ButtonAction {
    public:
      ResetRegisters(Crate * crate);
      void respond(xgi::Input * in, ostringstream & out);
    };      
    /**************************************************************************
     * ResetRegisters
     *
     * A small class to reprogram the DCFEB
     **************************************************************************/
    class ReprogramDCFEB : public ButtonAction {
    public:
      ReprogramDCFEB(Crate * crate);
      void respond(xgi::Input * in, ostringstream & out);
    };      
  }
}

#endif //__emu_odmbdev_Buttons_h__
