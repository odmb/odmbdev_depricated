#include "emu/odmbdev/Manager.h"
#include "emu/odmbdev/Buttons.h"
#include "emu/odmbdev/ButtonAction.h"

#define USE_CRATE_N 0 // ignore anything but the first crate
//#define XML_CONFIGURATION_FILE "/homes/fgolf/TriDAS/emu/odmbdev/xml/crate-ucsb.xml"
//#define XML_CONFIGURATION_FILE "/local/data/odmb_ucsb/odmbdev/xml/crate-ucsb.xml"
#define XML_CONFIGURATION_FILE "xml/crate-ucsb.xml"

using namespace cgicc;

namespace emu { namespace odmbdev {

    /**************************************************************************
     * The Constructor
     *
     * Here is where you can add actions or common actions. Buttons are buttons
     * or buttons with text fields that are displayed on the page. Common
     * actions are actions which scroll in pace with the page and float to the
     * right hand side of the page. They are useful for common actions that
     * should always be visible, such as hard reset.
     *************************************************************************/
    Manager::Manager( xdaq::ApplicationStub *s ) :
      xdaq::WebApplication( s ),
      webOutputLog(),
      logger_( Logger::getInstance( generateLoggerName() ) )
    {
      cout<<"Parsing XML_CONFIGURATION_FILE"<<endl;
      XMLParser xmlparser;
      xmlparser.parseFile(XML_CONFIGURATION_FILE);

      cout<<"Starting Manager"<<endl;
      if (!xmlparser.GetEmuEndcap()) {
        // if something went wrong while parsing ...
        XCEPT_RAISE(xcept::Exception,
                    string("Could not parse xml crate configuration file, ") +
                    XML_CONFIGURATION_FILE + ".");
      }

      cout<<"Loading crate"<<endl;
      Crate * crate = xmlparser.GetEmuEndcap()->crates().at(USE_CRATE_N);

      /************************************************************************
       * The Common Buttons, which are always available on the right hand-side
       * of the page.
       *
       ***********************************************************************/

//       addCommonActionByTypename<HardReset>(crate);
//       addCommonActionByTypename<L1Reset>(crate);
//       addCommonActionByTypename<BC0>(crate);

      /************************************************************************
       * The Buttons, which are listed in the below order on the web page.
       *
       ***********************************************************************/

//       addActionByTypename<ReadBackUserCodes>(crate);
      cout<<"Adding slot button"<<endl;
      addActionByTypename<ChangeSlotNumber>(crate);
      cout<<"Adding button"<<endl;
      addActionByTypename<ExecuteVMEDSL>(crate);
      cout << "Adding test reset button" << endl;
      addActionByTypename<ResetRegisters>(crate);
      cout << "Adding reprogram DCFEB button" << endl;
      addActionByTypename<ReprogramDCFEB>(crate);
      cout << "Adding button to program ODMB FW via VME" << endl;
      addActionByTypename<LoadMCSviaBPI>(crate, this);
      
      /************************************************************************
       * Log Buttons
       *
       * These are used for maintaining the log displayed on the web page. You
       * probably don't need to modify these. However, it would be neat for
       * someone to add a write to file button.
       ***********************************************************************/

      addLogActionByTypename<ClearLog>(crate);

      bindWebInterface();
    }


    void Manager::bindWebInterface()
    {
      xgi::bind( this, &Manager::defaultWebPage, "Default" );
      //xgi::bind( this, &Manager::commonActionsCallback, "commonActions" );
      xgi::bind( this, &Manager::actionsCallback, "actions" );
      xgi::bind( this, &Manager::logActionsCallback, "logActions" );
    }

    void Manager::defaultWebPage(xgi::Input *in, xgi::Output *out)
    {
      cout<<"Starting defaultWebPage"<<endl;
      *out << HTMLDoctype(HTMLDoctype::eStrict)
           << endl
           << endl
           << html().set("lang", "en")
                    .set("dir","ltr")
           << head()
           << style().set("rel", "stylesheet").set("type", "text/css")<<endl
           << "" // you could add page-wide styles here
           << style()
           << script().set("type", "text/javascript")
           // I appologize to the programming gods for writing JavaScript as
           // a string inside a C++ program ... (NB gcc will concatenate
           // adjacent string literals)
           << "function toggleSidebox() {"<<endl
           << "  var elements = document.getElementsByClassName('sidebox');"<<endl
           << "  Array.prototype.slice.call(elements, 0).map("
              "    function (e) { "
              "      e.style.display = e.style.display == 'none' ? 'block' "
              "                                                  : 'none'"
              "    })"
              "}"
           << script()
           << head()<<endl
           << body().set("style",
                         string("padding-bottom: 10em; ")
                         + "color: #333; ")
 	   << cgicc::div().set("style",
			       string("width: 515px;") +          
			       "float: left")<<endl
	   << h1()
           << "ODMB Test Routines - UCSB"
	   << h1();
      
      
      // Manuel: html object to browse local drive
      //*out<<endl<<"Select file with VME commands: <input type=file name=VMEfile  size=50 />"<<endl;
      
      // this is only for common actions which we always want visible
      for(unsigned int i = 0; i < commonActions.size(); ++i) {
	
        commonActions[i]->display(out);
	
        // and here we close the form
        *out << cgicc::form()
             << p()
             << endl;
      }

      //      *out << cgicc::div(); // end: floating right hand side box

      // most actions will appear here
      for(unsigned int i = 0; i < actions.size(); ++i) {
        // this multi-line statement sets up a form for the action,
        // which will create buttons, etc. The __action_to_call hidden
        // form element tells the Manager which action to use when
        // this form is submitted.
        *out << p()
             << cgicc::form().set("method","GET")
                             .set("action", "actions")
             << cgicc::input().set("type","hidden")
                              .set("value",numberToString(i))
                              .set("name","__action_to_call")
             << endl;

        actions[i]->display(out);

        // and here we close the form
        *out << cgicc::form()
             << p()
             << endl;
      }

      *out << cgicc::div();

      *out << cgicc::div().set("style", string("margin-left: 525px;") + "padding-left: 30px;"+ "padding-right: 30px;");

      for(unsigned int i = 0; i < logActions.size(); ++i) { // display log buttons at the top
        *out << p()
	     << cgicc::form().set("method","GET")
	  .set("action", "logActions")
	     << "Output log " 
             << cgicc::input().set("type","hidden")
	  .set("value",numberToString(i))
	  .set("name","__action_to_call");
	
	logActions[i]->display(out);
	
	*out << cgicc::form()
	     << p();
      }

      *out << textarea().set("style",
                             string("width: 100%; ")
                             + "height: 930px; ")
           // NB, I purposely called .str(), I don't want to remove all the
           // contents of the log into the web page, I want them to persist
           << this->webOutputLog.str()
           << textarea();

      *out << cgicc::div()
           << body() << html();

      //      Manager::setSlotNumber(15);
    }

    void Manager::commonActionsCallback(xgi::Input *in, xgi::Output *out)
    {
      int action_to_call = getFormValueInt("__action_to_call", in);

      ostringstream action_output;

      commonActions.at(action_to_call)->respond(in, action_output);

      this->webOutputLog << action_output.str();

      BackToMainPage(in, out);
    }

    void Manager::actionsCallback(xgi::Input *in, xgi::Output *out)
    {
      int action_to_call = getFormValueInt("__action_to_call", in);

      ostringstream action_output;

      actions.at(action_to_call)->respond(in, action_output);

      this->webOutputLog << action_output.str();

      BackToMainPage(in, out);
    }

    void Manager::logActionsCallback(xgi::Input *in, xgi::Output *out)
    {
      int action_to_call = getFormValueInt("__action_to_call", in);

      ostringstream action_output;

      logActions.at(action_to_call)->respond(in, action_output, this->webOutputLog);

      this->webOutputLog << action_output.str();

      BackToMainPage(in, out);
    }

    void Manager::addAction(shared_ptr<Action> act) {
      actions.push_back(act);
    }

    template <typename T>
    void Manager::addActionByTypename(Crate * crate) {
      actions.push_back(shared_ptr<T>(new T(crate)));
    }

    template <typename T>
    void Manager::addActionByTypename(Crate * crate, emu::odmbdev::Manager* manager ) {
      actions.push_back(shared_ptr<T>(new T(crate, manager)));
    }

    void Manager::addCommonAction(shared_ptr<Action> act) {
      commonActions.push_back(act);
    }

    template <typename T>
    void Manager::addCommonActionByTypename(Crate * crate) {
      commonActions.push_back(shared_ptr<T>(new T(crate)));
    }

    void Manager::addLogAction(shared_ptr<LogAction> act) {
      logActions.push_back(act);
    }

    template <typename T>
    void Manager::addLogActionByTypename(Crate * crate) {
      logActions.push_back(shared_ptr<T>(new T(crate)));
    }

    int Manager::getFormValueInt(const string form_element, xgi::Input *in)
    {
      const cgicc::Cgicc cgi(in);
      int form_value;
      cgicc::const_form_iterator name = cgi.getElement(form_element);
      if(name !=cgi.getElements().end())
        {
          form_value = cgi[form_element]->getIntegerValue();
        }
      else
        {
          XCEPT_RAISE( xcept::Exception, "Form element, " + form_element + ", was not found." );
        }
      return form_value;
    }

    void Manager::BackToMainPage(xgi::Input * in, xgi::Output * out ) // Redirect back to the main page. -Joe
    {
      //// Use this after a "GET" button press to get back to the base url
      *out << HTMLDoctype(HTMLDoctype::eStrict)
           << endl
           << html().set("lang", "en")
                           .set("dir","ltr")
           << endl
           << head()
           << meta().set("http-equiv","Refresh").set("content","0; url=./")
           << head()
           << endl
           << body()
           << p() << "Operation Complete" << cgicc::p()
           << body()
           << endl
           << html()
           << endl;
    }

    template <typename T>
    string Manager::numberToString(T number) {
      stringstream convert;
      convert << number;
      return convert.str();
    }

    string Manager::generateLoggerName()
    {
      xdaq::ApplicationDescriptor *appDescriptor = getApplicationDescriptor();
      string                      appClass       = appDescriptor->getClassName();
      unsigned long               appInstance    = appDescriptor->getInstance();
      stringstream                ss;
      string                      loggerName;

      ss << appClass << appInstance;
      loggerName = ss.str();

      return loggerName;
    }

    /**
     * Provides the factory method for the instantiation of applications.
     */
    XDAQ_INSTANTIATOR_IMPL(Manager)
  }
}
