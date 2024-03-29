#ifndef __emu_odmbdev_Action_h__
#define __emu_odmbdev_Action_h__

#include "emu/pc/Crate.h"
#include "emu/pc/CFEB.h"
#include "emu/pc/DAQMB.h"
#include "emu/pc/CCB.h"
#include "emu/pc/DDU.h"
#include "emu/pc/TMB.h"
#include "xgi/Method.h"
#include "cgicc/HTMLClasses.h"
#include "xcept/tools.h"
#include "xcept/Exception.h"

#include <iostream>
#include <sstream>
#include <vector>

/******************************************************************************
 * The Action Class
 *
 * The action class is the superclass/interface for all actions. Read below for
 * information on how to override it properly. The important methods to
 * override are display and respond, but the compiler should complain if you do
 * not do so.
 *****************************************************************************/

using namespace std;
using namespace emu::pc;

namespace emu { 

    namespace pc {
        class Crate;
        class CFEB;
        class DAQMB;
        class CCB;
        class DDU;
        class TMB;
        class ALCTController;
        class ODAQMB;
    }

  namespace odmbdev {

    class Manager;

        class Action {
        public:
            Action(Crate * crate);
	    Action(Crate * crate, Manager * manager);
            // a virtual destructor removes a warning about have a class with virtual
            // methods but a non-virtual destructor
            virtual ~Action() { };

            // virtual with "= 0" indicates that the method *must* be implemented by
            // any subclasses this ensures that this method can be called on
            // objects of type Action
            // http://en.wikibooks.org/wiki/C%2B%2B_Programming/Classes/Abstract_Classes

            /************************************************************************
             * display
             *
             * This method should be overridden to change how this action is
             * presented on the web page. This class contains a number of static
             * methods that you can use to ease this job (e.g. AddButton and
             * friends).
             *
             * Use classes in the cgicc namespace (e.g. input, p) as arguments to the
             * stream operator of the `out' parameter. 
             ***********************************************************************/
            virtual void display(xgi::Output * out) = 0;

            /************************************************************************
             * respond
             *
             * This method should be overridden to change how this action responds to
             * being called. Any input values which you specified in display can be
             * accessed here with the `getFormValue____' static methods.
             *
             * The out parameter can be used to print to the log displayed on the web
             * page.
             *
             * You'll also have access to the fields listed below in protected. In
             * particular, you will probably find dmbs, tmb, ccb, and ddus useful.
             *
             * Finally, note that we've already defined type aliases for the cfeb
             * iterators.
             ***********************************************************************/
            virtual void respond(xgi::Input * in, ostringstream & out) = 0;

        protected:
            typedef vector<CFEB>::iterator CFEBItr;
            typedef vector<CFEB>::reverse_iterator CFEBrevItr;


	    Crate * crate;
	    std::vector<DAQMB*> dmbs;
	    std::vector<DDU*> ddus;
	    TMB* tmb;
	    CCB* ccb;
	    ALCTController* alct;
	    Manager* manager;

            static int getFormValueInt(const string form_element, xgi::Input *in);
            static int getFormValueIntHex(const string form_element, xgi::Input *in);
            static float getFormValueFloat(const string form_element, xgi::Input *in);
            static string getFormValueString(const string form_element, xgi::Input *in);
            static void getFormValueFile(const string form_element, xgi::Input *in, std::ostream& outstream);

            static void AddButton(xgi::Output *out,
                                  const string button_name)
                throw (xgi::exception::Exception);
            static void AddButtonWithTextBox(xgi::Output *out,
                                             const string button_name,
                                             const string textboxname,
                                             const string textbox_default_value)
                throw (xgi::exception::Exception);
            static void AddButtonWithTwoTextBoxes(xgi::Output *out,
                                                  const string button_name,
                                                  const string textboxname1,
                                                  const string textbox_default_value1,
                                                  const string textboxname2,
                                                  const string textbox_default_value2)
                throw (xgi::exception::Exception);
            static void AddButtonWithLongTextBox(xgi::Output *out,
                                                 const string button_name,
                                                 const string textboxname,
                                                 const string textbox_default_value)
                throw (xgi::exception::Exception);

	    static void AddButtonWithFourTextBoxes(xgi::Output *out,
						    const string button_name,
						    const string textboxname1,
						    const string textbox_default_value1,
						    const string textboxname2,
						    const string textbox_default_value2,
						    const string textboxname3,
						    const string textbox_default_value3,
						    const string textboxname4,
						    const string textbox_default_value4)
                throw(xgi::exception::Exception);

            // static void AddButtonWithFileUpload(xgi::Output *out,
            //                                     const string button_name)
            //     throw (xgi::exception::Exception);
        };
    }
}

#endif
