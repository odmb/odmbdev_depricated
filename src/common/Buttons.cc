#include "emu/odmbdev/Buttons.h"

#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

/******************************************************************************
 * Some classes are declared in the header file because they are short and
 * sweet. Check there first!
 *
 * Also note, at the end of this file is a template for new Action
 * sub-classes.
 * 
 *****************************************************************************/

namespace emu { 
  namespace odmbdev {

    std::string ToUpper(std::string s){
      for(unsigned int i(0); i<s.length(); ++i){
	s.at(i)=toupper(s.at(i));
      }
      return s;
    }

    string FixLength(unsigned int Number, unsigned int Length, bool isHex){
      std::stringstream Stream;
      if(isHex) Stream << std::hex << Number;
      else Stream << std::dec << Number;
      string sNumber = Stream.str();
      for(unsigned int cha=0; cha<sNumber.size(); cha++) sNumber[cha] = toupper(sNumber[cha]);
      while(sNumber.size() < Length) sNumber = " " + sNumber;
      return sNumber;
    }

    unsigned short CountSetBits(unsigned short x){
      unsigned short count(0);
      for(count =0; x; count++){
	x &= x-1;
      }
      return count;
    }

    /**************************************************************************
     * ResetRegisters
     *
     * A small class to implement a reset from the ODMB_CTRL bits --TD
     **************************************************************************/
    ResetRegisters::ResetRegisters(Crate * crate) 
      : ButtonAction(crate,"Reset ODMB registers/FIFOs") 
    { /* The choices here are really a blank constructor vs duplicating the ExecuteVMEDSL constructor.
	 I've tried the former -- TD
      */
    }
    
    void ResetRegisters::respond(xgi::Input * in, ostringstream & out) { // TD
      // ButtonAction::respond(in, out);
      out << "********** VME REGISTER RESET **********" << endl;
      bool debug = false;
      int slot = 15;
      unsigned int shiftedSlot = slot << 19;
      char rcv[2];
      // These are the appropriate R/W addresses for register reset
      unsigned int read_addr = 0x003004;
      unsigned int write_addr = 0x003000;
      // Set the top bits of address to the slot number
      read_addr = (read_addr & 0x07ffff) | shiftedSlot;
      write_addr = (write_addr & 0x07ffff) | shiftedSlot;
      unsigned short int reset_command = 0x100;
      unsigned short int data;
      if (debug) out << "data initialized to " << hex << data << endl;
      
      // Read = 2
      // Write = 3
      
      if (debug) printf("Calling:  vme_controller(%d,%06x,&%04x,{%02x,%02x})  \n", 2, 
			(read_addr & 0xffffff), (data & 0xffff), (rcv[0] & 0xff), (rcv[1] & 0xff));      
      crate->vmeController()->vme_controller(2,read_addr,&data,rcv);
      unsigned short int VMEresult = (rcv[1] & 0xff) * 0x100 + (rcv[0] & 0xff);
      if (debug) {
	out << "read: " << FixLength(VMEresult) << endl;
	printf("Calling:  vme_controller(%d,%06x,&%04x,{%02x,%02x})  \n", 3, 
	       (write_addr & 0xffffff), (reset_command & 0xffff), (rcv[0] & 0xff), (rcv[1] & 0xff));
      }
      crate->vmeController()->vme_controller(3,write_addr,&reset_command,rcv);
      usleep(100);      
      if (debug) printf("Calling:  vme_controller(%d,%06x,&%04x,{%02x,%02x})  \n", 3, 
			(write_addr & 0xffffff), (VMEresult & 0xffff), (rcv[0] & 0xff), (rcv[1] & 0xff));
      crate->vmeController()->vme_controller(3,write_addr,&VMEresult,rcv);
      usleep(100);
      if (debug) printf("Calling:  vme_controller(%d,%06x,&%04x,{%02x,%02x})  \n", 2, 
			(read_addr & 0xffffff), (data & 0xffff), (rcv[0] & 0xff), (rcv[1] & 0xff));            
      crate->vmeController()->vme_controller(2,read_addr,&data,rcv);
      VMEresult = (rcv[1] & 0xff) * 0x100 + (rcv[0] & 0xff);

      out << "R  " << FixLength(read_addr & 0xffff) << "        " << FixLength(VMEresult)  << endl;      
      
    }

    /**************************************************************************
     * ResetRegisters
     *
     * A small class to implement a reset from the ODMB_CTRL bits --TD
     **************************************************************************/
    ReprogramDCFEB::ReprogramDCFEB(Crate * crate) 
      : ButtonAction(crate,"Reprogram DCFEBs") 
    { /* The choices here are really a blank constructor vs duplicating the ExecuteVMEDSL constructor.
	 I've tried the former -- TD
      */
    }
    
    void ReprogramDCFEB::respond(xgi::Input * in, ostringstream & out) { // TD
      out << "********** VME REGISTER RESET **********" << endl;
      int slot = 15;
      unsigned int shiftedSlot = slot << 19;
      char rcv[2];
      // These are the appropriate R/W addresses for register reset
      unsigned int write_addr = 0x003010;
      // Set the top bits of address to the slot number
      write_addr = (write_addr & 0x07ffff) | shiftedSlot;
      unsigned short int reset_command = 0x1;
      
      crate->vmeController()->vme_controller(3,write_addr,&reset_command,rcv);
      usleep(100);      

      out << "W   3010      1          Reprogram all DCFEBs" << endl;      
      
    }
    
    /**************************************************************************
     * ExecuteVMEDSL
     *
     * A domain-specific-lanaguage for issuing vme commands. 
     *************************************************************************/

    ExecuteVMEDSL::ExecuteVMEDSL(Crate * crate)
      : ThreeTextBoxAction(crate, "Run VME commands")
    {
      cout << " Initializing the DAQMBs, which are VMEModules" << endl; 
      for(vector <DAQMB*>::iterator dmb = dmbs.begin();dmb != dmbs.end(); ++dmb) 
	(*dmb)->start();
      nCommand = 0;
    }
    
    void ExecuteVMEDSL::respond(xgi::Input * in, ostringstream & out) {
      ThreeTextBoxAction::respond(in, out);
      
      nCommand++;
      out<<"****************   VME command "<<nCommand<<"   ***************"<<endl;
      int slot = 15; // hard code a default VME slot number

      //// the arguments for vme_controller ////
      char rcv[2];
      unsigned int sleepTimer;
      unsigned int testReps(0);
      unsigned int addr;
      unsigned short int data;
      //      out << "data initialized to " << hex << data << endl;
      int irdwr, TypeCommand=0; 
      // Taking negative irdwr and TypeCommand to represent commands not passed to VME (for loops, etc.) - AD
      //// From VMEController.cc:
      // irdwr:   
      // 0 bufread
      // 1 bufwrite 
      // 2 bufread snd  
      // 3 bufwrite snd 
      // 4 flush to VME (disabled)
      // 5 loop back (disabled)
      // 6 delay

      //////////////////////////////////////////////////////  
      // Open a new output logfile today - KF

      time_t rawtime;
      struct tm *timeinfo;
      time(&rawtime);
      timeinfo = localtime(&rawtime);

      char outputfilename[80];
      strftime( outputfilename, 80, "logfiles/%y_%m_%d_ODMB_testing.log", timeinfo );

      char timestamp[80];
      strftime( timestamp, 80, "%T", timeinfo );

      struct stat statbuffer;
      int writeheader = stat( outputfilename, &statbuffer );
	   
      ofstream logfile;
      logfile.open( outputfilename, ios::app );
      if( writeheader!=0 ) logfile << "# R/W\tCommand\tData\tSlot\tTime\t\tComments" << endl;

      ////////////////////////////////////////////////////// 
      istringstream countertext(this->textBoxContent1); //read the number of times to repeat commands and possibly a file containing commands  
      istringstream filetext(this->textBoxContent2); // input a file path to the wimp page, read commands from the file
      bool textFileMode(false); // Option to input a text file from the second box on WIMP
      // works with file in current directory or full path - JB-F
      string filePath, fileContents, file_line;
      if (filetext.str().size()) { // If you specify a file path on the WIMP page, then we feed it into the string "fileContents" - JB-F
      	textFileMode = true;
      	filePath = filetext.str();
      	ifstream fileFromWIMP(filePath.c_str());
      	while ( fileFromWIMP.good() ) {
          getline (fileFromWIMP,file_line);
      	  fileContents+=file_line;
      	  fileContents+="\n";
    	}
    	fileFromWIMP.close();
      }
      istringstream alltext; // where the input commands will go
      if (textFileMode) {
	alltext.str(fileContents); // assigns content of text file to alltext, after you push "Execute VME DSL Program" - JB-F
      } else {
      	alltext.str(this->textBoxContent3); // get the text from the box                                                                                                      
      }
      string line, buffer;
      getline(countertext,line,'\n');
      const unsigned long repeatNumber=strtoul(line.c_str(),NULL,0);
      std::vector<std::string> allLines(0);
      while (getline(alltext,line,'\n')){
	if(1){//line.size()){//Discard empty lines
	  allLines.push_back(line);
	}
      }
      bool writeHex = true;

      for(unsigned int repNum=0; repNum<repeatNumber; ++repNum){
	unsigned int loopLevel(0);
	std::vector<unsigned int> loopCounter(0), loopMax(0), loopStart(0);
	for(unsigned int lineNum=0; lineNum<allLines.size(); ++lineNum){
	  line=allLines.at(lineNum);
	  while(line.length()>0 && (line.at(0)==' ' || line.at(0)=='\t')) line.erase(0,1);
	  istringstream iss(line);

	  // 0 :       End of run, stop execution
	  // 1 :       Binary (simulation file style)
	  // 2,r,R :   Reads in hex
	  // 3,w,W :   Writes in hex
	  // 4,rs,RS : Reads in hex
	  // 5,ws,WS : Writes in hex
	  // 6,bl,BL : Begin loop
	  // 7,el,EL : End loop
	  // 
	  // >3 : Produces a sleep for that number of microseconds
	  // 
	  // <0 : Send an error message to the output and abort further execution.
	  if(line.size()>1) iss >> buffer;
	  else buffer = "#";  // This avoids problems when the line is too short
	  buffer=ToUpper(buffer);
	  if(buffer=="0")  {
	    out<<"Found EOR, exiting."<<endl;
	    break;// EOR instruction
	  } else if(buffer=="1") { // Expect 32 bits for address and 32 bits for data
	    string addr_str, data_str, tmp_str;
	    //	    out << "data from string: " << hex << data << endl;

	    while(addr_str.size()<32 && iss.good()){ // read in 32 bits for address
	      iss >> tmp_str;
	      addr_str += tmp_str;
	      //	      out<<"addr_str:"<<addr_str<<endl;
	    }
	    while(data_str.size()<32 && iss.good()){ // read in 32 bits for data
	      iss >> tmp_str;
	      data_str += tmp_str;
	      //	      out<<"data_str:"<<data_str<<endl;
	    }
	    if(addr_str.size()!=32 || data_str.size()!=32){
	      out<<"ERROR: address("<<addr_str<<") or data("<<data_str
		 <<") is not 32 bits on line: "<<line<<endl;
	      return;
	    }
	    // 26th and 25th "bits" from right tell read (10) or write (01)
	    irdwr = (addr_str.at(addr_str.size()-26)=='1')? 2 : 3; 
	    addr = BinaryString_to_UInt(addr_str);
	    //	    out << "data = " << data << endl;
	    data = BinaryString_to_UInt(data_str);
	    //	    out << "data = " << data << endl;
	    TypeCommand = irdwr;
	  } else if(buffer=="2" || buffer=="R") { // Read in hex
	    //	    out << "data = " << hex << data << endl;
	    iss >> hex >> addr >> hex >> data;	
	    //	    out << "data = " << hex << data << endl;
	    irdwr = 2; TypeCommand = 2;
	  } else if(buffer=="3" || buffer=="W") { // Write in hex
	    iss >> hex >> addr;	
	    if(addr >= 0x4000 && addr <= 0x4018) {iss >> dec >> data; writeHex = false;}
	    else {iss >> hex >> data; writeHex = true;}
	    irdwr = 3; TypeCommand = 3;
	  } else if(buffer=="4" || buffer=="RS") { // Read in hex with slot
	    iss >> hex >> addr >> hex >> data >> dec >> slot;	
	    irdwr = 2; TypeCommand = 4;
	  } else if(buffer=="5" || buffer=="WS") { // Write in hex with slot
	    iss >> hex >> addr;	
	    if(addr >= 0x4000 && addr <= 0x4018) {iss >> dec >> data >> dec >> slot; writeHex = false;}	
	    else {iss >> hex >> data >> dec >> slot; writeHex = true;}
	    irdwr = 3; TypeCommand = 5;
	  } else if(buffer=="6" || buffer=="BL") {
	    ++loopLevel;
	    loopCounter.push_back(1);
	    int thisLoopMax(0);
	    iss >> dec >> thisLoopMax;
	    loopMax.push_back(thisLoopMax);
	    loopStart.push_back(lineNum);
	    irdwr = -1; TypeCommand= -1; //Negative; do not send to VME - AD
	  } else if(buffer=="7" || buffer=="EL") {
	    if(loopCounter.at(loopLevel-1)<loopMax.at(loopLevel-1)){
	      lineNum=loopStart.at(loopLevel-1);
	      ++loopCounter.at(loopLevel-1);
	    }else{
	      loopCounter.pop_back();
	      loopMax.pop_back();
	      loopStart.pop_back();
	      --loopLevel;
	    }
	    irdwr = -1; TypeCommand = -1; //Negative; do not send to VME - AD
	  } else if(buffer=="8" || buffer=="SLEEP" || buffer=="WAIT") {
	    iss >> sleepTimer;
	    usleep(sleepTimer);
	    continue; // Nothing else to do from this line.
	  } else if(buffer=="RL1A") {
	    TypeCommand = 9;
	    iss >> sleepTimer;
	  } else if(buffer=="RL1AM") {
	    TypeCommand = 10;
	    iss >> sleepTimer;
	  } else if(buffer=="COMP_DDU") {
	    TypeCommand = 11;
	    iss >> testReps;
	  } else if(buffer=="COMP_PC"){
	    TypeCommand = 12;
	    iss >> testReps;
	  } else { // Anything else is treated as a comment.
	    out  << line << endl;
	    logfile << "# " << line << endl;
	    continue; // Next line, please.
	  }

	  //// We are at the end of official commands, add the rest to comments - KF
	  string comments;
	  getline( iss, comments );
	  while(comments.size()>0 && (comments.at(0)==' ' || comments.at(0)=='\t')) comments.erase(0,1);
	  //// Output type of command is always W or R - KF
	  //// Added command types L for loops, T for tests - AD
	  string commandtype;
	  switch(irdwr){
	  case 2:
	    commandtype="R";
	    break;
	  case 3:
	    commandtype="W";
	    break;
	  case -1:
	    commandtype="L";
	    break;
	  case 11:
	  case 12:
	    commandtype="T";
	    break;
	  default:
	    commandtype="Error with command type!";
	    break;
	  }

	  //// If we make it here, we have a VME command to run! ////
	  //Only if TypeCommand>0 due to addition of loop! - AD
	  //printf("TypeCommand: %d\n", TypeCommand);
	  //out << TypeCommand << endl;

	  // Set the top bits of address to the slot number
	  unsigned int shiftedSlot = slot << 19;
	  int nDigits = 5;
	  if(TypeCommand>=2 && TypeCommand<=5){
	    addr = (addr & 0x07ffff) | shiftedSlot;	    
	    printf("Calling:  vme_controller(%d,%06x,&%04x,{%02x,%02x})  ", irdwr, 
		   (addr & 0xffffff), (data & 0xffff), (rcv[0] & 0xff), (rcv[1] & 0xff));
	    crate->vmeController()->vme_controller(irdwr, addr, &data, rcv); // Send the VME command!
	    usleep(1);
	    
	    // If it was a read, then show the result
	    if(irdwr==2) printf("  ==> rcv[1,0] = %02x %02x", (rcv[1] & 0xff), (rcv[0] & 0xff));
	    printf("\n");
	    fflush(stdout);
	    
	    
	    // Output to website
	    addr = (addr & 0x07ffff);  // Masks out the slot number
	    unsigned int VMEresult = (rcv[1] & 0xff) * 0x100 + (rcv[0] & 0xff);
	    bool readHex = true;
	    if((addr >= 0x321C && addr <= 0x337C) || (addr >= 0x33FC && addr <= 0x367C) ||
	       (addr >= 0x4400 && addr <= 0x4418) 
	       || addr == 0x500C || addr == 0x510C || addr == 0x520C || addr == 0x530C || addr == 0x540C 
	       || addr == 0x8004 ||  (addr == 0x5000 && VMEresult < 0x1000)) readHex = false;
	    switch (irdwr) {
	    case 2:
	      out << "R  " << FixLength(addr) << "        "  << FixLength(VMEresult, nDigits, readHex)  << "    "<<comments<<endl;
	      logfile << "R  " << FixLength(addr) << "        "  << FixLength(VMEresult, nDigits, readHex)  << "    "
		      << timestamp << "\t" << comments<<endl;
	      break;
	    case 3:
	      out << "W  " << FixLength(addr) << "  " << FixLength(data, nDigits, writeHex) <<  "          "<<comments<<endl;
	      logfile << "W  " << FixLength(addr) << "  " << FixLength(data, nDigits, writeHex) <<  "          "
		      << timestamp << "\t" << comments<<endl;
	      break;
	    }
	  } else if (TypeCommand==9 || TypeCommand==10){
	    unsigned int read_fifo = 0x005000;
	    unsigned int reset_fifo = 0x005020;
	    // Set the top bits of address to the slot number
	    read_fifo = (read_fifo & 0x07ffff) | shiftedSlot;
	    reset_fifo = (reset_fifo & 0x07ffff) | shiftedSlot;
	    crate->vmeController()->vme_controller(2, read_fifo, &data, rcv); // Read the 1st word for L1A_MATCH
	    if(TypeCommand == 9) crate->vmeController()->vme_controller(2, read_fifo, &data, rcv); //2nd word for L1A
	    unsigned int VMEresult = (rcv[1] & 0xff) * 0x100 + (rcv[0] & 0xff);
	    usleep(1);
	    
	    data = 0xff;
	    crate->vmeController()->vme_controller(3, reset_fifo, &data, rcv); // Send the VME command!
	    usleep(1);
	    long l1a_cnt = VMEresult;
	    string command = "RL1A ", l1a_comment = "L1A";
	    if(TypeCommand == 10) {command = "RL1AM";l1a_comment = "L1A_MATCH";}
	    if(l1a_cnt < 0x1000){
	      out << command<<"           "  << FixLength(l1a_cnt, nDigits, false)  << "    "<<comments<<endl;
	      logfile << command<< "           "  << FixLength(l1a_cnt, nDigits, false)  << "    "
		      << timestamp << "\t" << comments<<endl;
	    } else {
	      out << command<< "            XXXX"  << "    "<<"No good DCFEB "<<l1a_comment<<"_CNT read"<<endl;
	      logfile << command<< "            XXXX"  << "    "<< timestamp << "\t" <<"No good DCFEB "<<l1a_comment<<"_CNT read"<<endl;
	    }
	  }else if (TypeCommand==11 || TypeCommand==12){
	    std::string testType = TypeCommand==11 ? "DDU" : "PC ";
	    char rcv_tx[2], rcv_rx[2];
	    unsigned short rcv_tx_i(0), rcv_rx_i(0), rcv_diff_i(0);
	    unsigned long bitChanges(0), bitMatches(0);
	    unsigned short arg(0);
	    const unsigned int addr_tx = TypeCommand==11 ? (0x005300 & 0x07ffff) | shiftedSlot : (0x005100 & 0x07ffff) | shiftedSlot;
	    const unsigned int addr_rx = TypeCommand==11 ? (0x005400 & 0x07ffff) | shiftedSlot : (0x005200 & 0x07ffff) | shiftedSlot;
	    for(unsigned int j(0); j<testReps; ++j){
	      crate->vmeController()->vme_controller(2, addr_tx, &arg, rcv_tx); //Read TX FIFO
	      usleep(1);
	      crate->vmeController()->vme_controller(2, addr_rx, &arg, rcv_rx); //Read RX FIFO
	      usleep(1);
	      
	      //Combine into int
	      rcv_tx_i=(rcv_tx[1]*0x100) + rcv_tx[0];
	      rcv_rx_i=(rcv_rx[1]*0x100) + rcv_rx[0];
	      
	      //Compare FIFOs
	      rcv_diff_i=rcv_tx_i^rcv_rx_i;
	      
	      //Count bits set to 1 (where differences occur)
	      unsigned int setBits(CountSetBits(rcv_diff_i));
	      bitChanges+=setBits;
	      bitMatches+=16-setBits;
	    }
	    if(bitChanges){
	      out << "COMP_" << testType << FixLength(testReps, nDigits+2, false) << FixLength(bitChanges, nDigits+1, false) << "    Bit changes = " << bitChanges << ", bit matches = " << bitMatches << std::endl;
	      logfile << "COMP_" << testType << FixLength(testReps, nDigits+2, false) << FixLength(bitChanges, nDigits+1, false) << "    " << timestamp << "\tBit changes = " << bitChanges << ", bit matches = " << bitMatches << std::endl;
	    }else{
	      out << "COMP_" << testType << FixLength(testReps, nDigits+2, false) << FixLength(bitChanges, nDigits+1, false) << "    All " << bitMatches << " bits match" << std::endl;
	      logfile << "COMP_" << testType << FixLength(testReps, nDigits+2, false) << FixLength(bitChanges, nDigits+1, false) << "    " << timestamp << "\tAll " << bitMatches << " bits match" << std::endl;
	    }
	  }
	} // while parsing lines
	out<<endl;
      }
      
      //// close the logfile -KF
      logfile.close();
      
    } // End ExecuteVMEDSL::respond
    
    

    //    /**************************************************************************
    //     * ActionTemplate
    //     *
    //     *************************************************************************/
    //
    //    ActionTemplate::ActionTemplate(Crate * crate)
    //      : Action(crate)
    //    { /* ... nothing to see here ... */ }
    //
    //    void ActionTemplate::display(xgi::Output * out) {
    //
    //    }
    //
    //    void ActionTemplate::respond(xgi::Input * in, ostringstream & out) {
    //
    //    }



    /**************************************************************************
     * Reads back the user codes
     *
     *************************************************************************/

    ReadBackUserCodes::ReadBackUserCodes(Crate * crate)
      : ButtonAction(crate, "Read back usercodes")
    { /* ... nothing to see here ... */ }


    void ReadBackUserCodes::respond(xgi::Input * in, ostringstream & out) {
      cout<<"*********************   Starting ReadBackUserCodes     *********************"<<endl;
      for(vector <DAQMB*>::iterator dmb = dmbs.begin();
	  dmb != dmbs.end();
	  ++dmb) {
	vector <CFEB> cfebs = (*dmb)->cfebs();
	for(CFEBrevItr cfeb = cfebs.rbegin(); cfeb != cfebs.rend(); ++cfeb) {
	  int cfeb_index = (*cfeb).number();

	  out << " ********************* " << endl
	      << " FEB" << cfeb_index << " : "
	      << " Usercode: " << hex << (*dmb)->febfpgauser(*cfeb) << endl
	      << " Virtex 6 Status: " << (*dmb)->virtex6_readreg(7);
	}

	(*dmb)->RedirectOutput(&out);
	(*dmb)->daqmb_adc_dump();
	(*dmb)->RedirectOutput(&cout);
      }
    }
  }
}
