#include "OnelabMessage.h"
#include "OnelabClients.h"
#include "StringUtils.h"
#include <algorithm>


class onelabMetaModelServer : public GmshServer{
 private:
  localNetworkSolverClient *_client;
 public:
  onelabMetaModelServer(localNetworkSolverClient *client)
    : GmshServer(), _client(client) {}
  ~onelabMetaModelServer(){}
  int NonBlockingSystemCall(const char *command)
  {
#if defined(WIN32)
    STARTUPINFO suInfo;
    PROCESS_INFORMATION prInfo;
    memset(&suInfo, 0, sizeof(suInfo));
    suInfo.cb = sizeof(suInfo);
    Msg::Info("Calling <%s>", command);
    // DETACHED_PROCESS removes the console (useful if the program to launch is
    // a console-mode exe)
    CreateProcess(NULL,command, NULL, NULL, FALSE,
		  NORMAL_PRIORITY_CLASS|DETACHED_PROCESS, NULL, NULL,
		  &suInfo, &prInfo);
    return 0;
#else
    if(!system(NULL)) {
      Msg::Error("Could not find /bin/sh: aborting system call");
      return 1;
    }
    std::string cmd(command);
    int pos;
    if((pos=cmd.find("incomp_ssh ")) != std::string::npos){
      cmd.assign(cmd.substr(pos+7));
      //cmd.append(" &>/dev/null & '");
      cmd.append(" &>onelab.log & '");
    }
    else
      cmd.append(" &");

    Msg::Info("Calling <%s>", cmd.c_str());
    return system(cmd.c_str());
#endif
  }// non blocking 
  int NonBlockingWait(int socket, double waitint, double timeout)
  {
    double start = GetTimeInSeconds();
    while(1){
      if(timeout > 0 && GetTimeInSeconds() - start > timeout)
        return 2; // timout
      // if(_client->getPid() < 0 || (_client->getCommandLine().empty() &&
      //                              !CTX::instance()->solver.listen))
      if(_client->getPid() < 0 || (_client->getCommandLine().empty()))
        return 1; // process has been killed or we stopped listening
      // check if there is data (call select with a zero timeout to
      // return immediately, i.e., do polling)
      int ret = Select(0, 0, socket);
      if(ret == 0){ // nothing available
        // if asked, refresh the onelab GUI
        std::vector<onelab::string> ps;
        onelab::server::instance()->get(ps, "Gmsh/Action");
        if(ps.size() && ps[0].getValue() == "refresh"){
          ps[0].setVisible(false);
          ps[0].setValue("");
          onelab::server::instance()->set(ps[0]);
          //onelab_cb(0, (void*)"refresh");
        }
        // wait at most waitint seconds and respond to FLTK events
        //FlGui::instance()->wait(waitint);
      }
      else if(ret > 0){
        return 0; // data is there!
      }
      else{
        // an error happened
        _client->setPid(-1);
        _client->setGmshServer(0);
        return 1;
      }
    }
  }
};

std::string localNetworkSolverClient::buildCommandLine(){
  std::string command = FixWindowsPath(getCommandLine());
  if(command.size()){
    std::vector<onelab::string> ps;
    get(ps, getName() + "/Action");
    std::string action = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/9CheckCommand");
    std::string checkCommand = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/9ComputeCommand");
    std::string computeCommand = (ps.empty() ? "" : ps[0].getValue());

    if(action == "initialize")
      command += " ";
    else if(action == "check")
      command += " " + getString("Arguments") + " " + checkCommand;
    else if(action == "compute")
      command += " " + getString("Arguments") + " " + computeCommand;
    else
      Msg::Fatal("localNetworkSolverClient::run: Unknown: Unknown Action <%s>", action.c_str());

    // append "-onelab" command line argument
    //command += " " + getSocketSwitch() + " \"" + getName() + "\"";
    command += " " + getSocketSwitch() + " " + getName() + " ";
  }
  return command;
}

bool localNetworkSolverClient::run()
{
 new_connection:
  _pid = 0;
  _gmshServer = 0;

  onelabMetaModelServer *server = new onelabMetaModelServer(this);
 
#if defined(WIN32)
  std::string socketName = ":";
#else
  std::string socketName = getUserHomedir() + ".gmshsock";
#endif
  socketName = ":";

  std::string sockname;
  std::ostringstream tmp;
  if(!strstr(socketName.c_str(), ":")){
    // Unix socket
    tmp << socketName << getId();
    sockname = FixWindowsPath(tmp.str());
  }
  else{
    // TCP/IP socket
    if(socketName.size() && socketName[0] == ':')
      tmp << GetHostName(); // prepend hostname if only the port number is given
    tmp << socketName ;
    sockname = tmp.str();
  }

  std::string command = buildCommandLine();

  std::cout << "sockname=<" << sockname << ">" << std::endl;
  // std::cout << "command=<" << command << ">" << std::endl;

  int sock;
  try{
    sock = server->Start(command.c_str(), sockname.c_str(), 10);
  }
  catch(const char *err){
    Msg::Error("%s (on socket '%s')", err, sockname.c_str());
    sock = -1;
  }

  if(sock < 0){
    server->Shutdown();
    delete server;
    return false;
  }

  Msg::StatusBar(2, true, "Now running client '%s'...", _name.c_str());
  while(1) {
    if(_pid < 0) break;
    
    int stop = server->NonBlockingWait(sock, 0.1, 0.);
    if(stop || _pid < 0) {
      Msg::Info("Stop=%d _pid=%d",stop, _pid);
      break;
    }
    int type, length, swap;
    if(!server->ReceiveHeader(&type, &length, &swap)){
      Msg::Error("Did not receive message header: stopping server");
      break;
    }
    // else
    //   std::cout << "FHF: Received header=" << type << std::endl;

    std::string message(length, ' ');
    if(!server->ReceiveMessage(length, &message[0])){
      Msg::Error("Did not receive message body: stopping server");
      break;
    }
    // else
    //   std::cout << "FHF: Received message=" << message << std::endl;

    switch (type) {
    case GmshSocket::GMSH_START:
      _pid = atoi(message.c_str());
      _gmshServer = server;
      break;
    case GmshSocket::GMSH_STOP:
      _pid = -1;
      _gmshServer = 0;
      break;
    case GmshSocket::GMSH_PARAMETER:
      {
        std::string version, type, name;
        onelab::parameter::getInfoFromChar(message, version, type, name);
        if(type == "number"){
          onelab::number p;
          p.fromChar(message);
          set(p);
        }
        else if(type == "string"){
          onelab::string p;
          p.fromChar(message);
          set(p);
        }
        else
          Msg::Fatal("FIXME query not done for this parameter type: <%s>", message.c_str());
      }
      break;
    case GmshSocket::GMSH_PARAMETER_QUERY:
      {
        std::string version, type, name;
        onelab::parameter::getInfoFromChar(message, version, type, name);
        if(type == "number"){
          std::vector<onelab::number> par;
          get(par, name);
          if(par.size() == 1){
            std::string reply = par[0].toChar();
            server->SendMessage(GmshSocket::GMSH_PARAMETER, reply.size(), &reply[0]);
          }
          else{
            std::string reply = "Parameter (number) " + name + " not found";
            server->SendMessage(GmshSocket::GMSH_INFO, reply.size(), &reply[0]);
          }
        }
        else if(type == "string"){
          std::vector<onelab::string> par;
          get(par, name);
          if(par.size() == 1){
            std::string reply = par[0].toChar();
            server->SendMessage(GmshSocket::GMSH_PARAMETER, reply.size(), &reply[0]);
          }
          else{
            std::string reply = "Parameter (string) " + name + " not found";
            server->SendMessage(GmshSocket::GMSH_INFO, reply.size(), &reply[0]);
          }
        }
        else
          Msg::Fatal("FIXME query not done for this parameter type: <%s>", message.c_str());
      }
      break;
    case GmshSocket::GMSH_PARAM_QUERY_ALL:
      {
        std::string version, type, name, reply;
        onelab::parameter::getInfoFromChar(message, version, type, name);
	if(type == "number"){
	  std::vector<onelab::number> numbers;
	  get(numbers, "");
	  for(std::vector<onelab::number>::iterator it = numbers.begin(); it != numbers.end(); it++){
	    reply = (*it).toChar();
	    server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_ALL, reply.size(), &reply[0]);
	  }
	  server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_END, 0, NULL);
	}
	else if(type == "string"){
	  std::vector<onelab::string> strings;
	  get(strings, "");
	  for(std::vector<onelab::string>::iterator it = strings.begin(); it != strings.end(); it++){
	    reply = (*it).toChar();
	    server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_ALL, reply.size(), &reply[0]);
	  }
	  server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_END, 0, NULL);
	}
        else
          Msg::Fatal("FIXME query not done for this parameter type: <%s>", message.c_str());
      }
      break;
    case GmshSocket::GMSH_PROGRESS:
      Msg::StatusBar(2, false, "%s %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_INFO:
      Msg::Direct("%-8.8s: %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_WARNING:
      Msg::Direct(2, "%-8.8s: %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_ERROR:
      //Msg::Direct(1, "%-8.8s: %s", _name.c_str(), message.c_str());
      Msg::Fatal("%-8.8s: %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_MERGE_FILE:
      Msg::Info("####Merge Post-Processing File %s",message.c_str());
      SystemCall("gmsh "+ message+" &");
      //implémentation pour le cas du loader en mode console.
      break;
    default:
      Msg::Warning("Received unknown message type (%d)", type);
      break;
    }
  }

  server->Shutdown();
  delete server;
  Msg::StatusBar(2, true, "Done running '%s'", _name.c_str());
  return true;
}

bool localNetworkSolverClient::kill()
{
  if(_pid > 0) {
    if(KillProcess(_pid)){
      Msg::Info("Killed '%s' (pid %d)", _name.c_str(), _pid);
      _pid = -1;
      return true; 
    }
  }
  _pid = -1;
  return false;
}

// PROMPTUSER

int PromptUser::getVerbosity(){
  std::vector<onelab::number> numbers;
  get(numbers,"VERBOSITY");
  if (numbers.size())
    return numbers[0].getValue();
  else
    return 0;
}
void PromptUser::setVerbosity(const int ival){
  onelab::number number;
  number.setName("VERBOSITY");
  number.setValue(ival);
  set(number);
}

void PromptUser::setNumber(const std::string paramName, const double val, const std::string &help){
  onelab::number number;
  number.setName(paramName);
  number.setValue(val);
  number.setHelp(help);
  set(number);
}

void PromptUser::addNumberChoice(std::string name, double val){
  std::vector<double> choices;
  std::vector<onelab::number> numbers;
  get(numbers, name);
  if(numbers.size())
    choices = numbers[0].getChoices();
  else{
    numbers.resize(1);
    numbers[0].setName(name);
  }
  numbers[0].setValue(val);
  choices.push_back(val);
  numbers[0].setChoices(choices);
  set(numbers[0]);
}

void PromptUser::addStringChoice(std::string name, std::string str){
  std::vector<std::string> choices;
  std::vector<onelab::string> strings;
  get(strings, name);
  if(strings.size())
    choices = strings[0].getChoices();
  else{
    strings.resize(1);
    strings[0].setName(name);
  }
  strings[0].setValue(str);
  choices.push_back(str);
  strings[0].setChoices(choices);
  set(strings[0]);
}

double PromptUser::getNumber(const std::string paramName){
  std::vector<onelab::number> numbers;
  get(numbers,paramName);
  if (numbers.size())
    return numbers[0].getValue();
  else
    Msg::Fatal("Unknown parameter %s",paramName.c_str());
}
bool PromptUser::existNumber(const std::string paramName){
  std::vector<onelab::number> numbers;
  get(numbers,paramName);
  return numbers.size();
}
void PromptUser::setString(const std::string paramName, const std::string &val, const std::string &help){
  onelab::string string;
  string.setName(paramName);
  string.setValue(val);
  string.setHelp(help);
  set(string);
}
std::string PromptUser::getString(const std::string paramName){
  std::vector<onelab::string> strings;
  get(strings,paramName);
  if (strings.size())
    return strings[0].getValue();
  else
    Msg::Fatal("Unknown parameter %s",paramName.c_str());
}
bool PromptUser::existString(const std::string paramName){
  std::vector<onelab::string> strings;
  get(strings,paramName);
  return strings.size();
}

std::string PromptUser::stateToChar(){
  std::vector<onelab::number> numbers;
  std::ostringstream sstream;
  get(numbers);
  for(std::vector<onelab::number>::iterator it = numbers.begin();
      it != numbers.end(); it++)
    sstream << (*it).getValue() << '\t';
  return sstream.str();
}

std::string PromptUser::showParamSpace(){
  std::string db = "ONELAB parameter space: size=" + itoa(onelab::server::instance()->getNumParameters()) + "\n";
  db.append(onelab::server::instance()->toChar());
  for(unsigned int i = 0; i < db.size(); i++)
    if(db[i] == onelab::parameter::charSep()) db[i] = '|';
  return db.c_str();
}

std::string PromptUser::showClientStatus(){
  std::ostringstream sstream;
  std::cout << "\nONELAB: Present state of the onelab clients" << std::endl;
  for(onelab::server::citer it = onelab::server::instance()->firstClient();
      it != onelab::server::instance()->lastClient(); it++){
    std::string name= it->second->getName();
    sstream << "<" << onelab::server::instance()->getChanged(name) << "> " << name << std::endl;
  }
  return sstream.str();
}

bool PromptUser::menu(std::string commandLine, std::string fileName, int modelNumber) { 
  int choice, counter1=0, counter2=0;
  std::string answ;
  std::string name;
  onelab::number x;
  onelab::string y;
  std::vector<onelab::number> numbers;
  std::vector<onelab::string> strings;

  std::string clientName="loadedMetaModel";
  EncapsulatedClient *loadedSolver = new  EncapsulatedClient(clientName,commandLine);
  //setString("Arguments/FileName",fileName);
  addStringChoice(clientName + "/InputFiles",fileName);

  do {
    std::cout << "\nONELAB: menu" << std::endl ;
    std::cout << " 1- View parameter space\n 2- Set a value\n 3- Analyze\n 4- Compute\n 5- List modified files\n 6- Client status\n 7- Quit metamodel" << std::endl;
    choice=0;
    std::string mystr;
    while( (choice<1 || choice>7) && ++counter1<10 ) {
      std::cout << "\nONELAB: your choice? "; 
      std::getline (std::cin, mystr);
      std::stringstream myStream(mystr);
      if (myStream >> choice) break;
      std::cout << "Invalid choice" << std::endl;
    }
    std::cout << "Your choice is <" << choice << ">" << std::endl;

    if (choice==1){
      // std::cout << "\nONELAB: Present state of the parameter space\n" << std::endl;
      // std::string db = onelab::server::instance()->toChar();
      // for(unsigned int i = 0; i < db.size(); i++)
      // 	if(db[i] == onelab::parameter::charSep()) db[i] = '|';
      // std::cout << db.c_str();
      std::cout << showParamSpace();
      choice=0;
    }
    else if (choice==2){
      std::cout << "ONELAB: Variable name? "; std::cin >> name;
      get(numbers,name);
      if (numbers.size()) {
	float fval;
	std::cout << "ONELAB: Value? "; std::cin >> fval;
	numbers[0].setValue(fval);
	bool allowed = set(numbers[0]);
      }
      else{
	get(strings,name);
	if (strings.size()) {
	  std::string sval;
	  std::cout << "ONELAB: Value? "; std::cin >> sval;
	  strings[0].setValue(sval);
	  bool allowed = set(strings[0]);
	  std::cout << "ONELAB: Ok you are allowed to modify this!" << std::endl;
	}
	else
	  std::cout << "ONELAB: The variable " << name << " is not defined" << std::endl;
      }
      choice=0;
    }
    else if (choice==3){
      loadedSolver->analyze();
      choice=0;
    }
    else if (choice==4){
      loadedSolver->compute();
      choice=0;
    }
    else if (choice==5){
      std::ifstream infile("onelab.modified");
      std::string buff;
      if (infile.is_open()){
	while ( infile.good() ) {
	  getline (infile,buff);
	  std::cout << buff << std::endl;
	}
      }
      choice=0;
    }
    else if (choice==6){
      std::cout << "\nONELAB: Present state of the onelab clients\n" << std::endl;
      for(onelab::server::citer it = onelab::server::instance()->firstClient();
	  it != onelab::server::instance()->lastClient(); it++){
	std::string name= it->second->getName();
	std::cout << name << "<" << onelab::server::instance()->getChanged(name) << ">" << std::endl;
      }
      choice=0;
    }
    else if (choice==7)
      exit(1);
    else
      choice=0;
  } while(!choice && ++counter2<20);
}

// LOCALSOLVERCLIENT

bool localSolverClient::checkCommandLine(){
  if(getCommandLine().empty()){
    std::string commandLine = getString("CommandLine");
    if(!commandLine.empty()){
      setCommandLine(commandLine);
    }
  }

  if(!getCommandLine().empty()){
    Msg::SetOnelabString(getName() + "/Action","initialize",false);
    run();
  }
  else{
       if(Msg::hasGmsh) {// exits metamodel and restores control to the onelab window
	Msg::Error("The command line of client <%s> is undefined.", getName().c_str());
	std::cout << "\n\nBrowse for client <" << getName() << "> executable in the ONELAB window.\n\n" << std::endl;
	return false;
      }
      else{ // asks the user in console mode
	std::cout << "\nONELAB:Enter the command line (with path) of the executable file of <" << getName() << ">" << std::endl;
	std::string cmdl;
	std::getline (std::cin,cmdl);
	setCommandLine(cmdl);
	return cmdl.size();
      }
    }
  return true;
}

const std::string localSolverClient::getString(const std::string what){
  std::vector<onelab::string> strings;
  std::string paramName(getName()+"/"+what);
  get(strings,paramName);
  if(strings.size())
    return strings[0].getValue();
  else
    return "";
}

const bool localSolverClient::getList(const std::string type, std::vector<std::string> &choices){
  std::vector<onelab::string> strings;
  get(strings,getName()+"/"+type);
  if(strings.size()){
    choices= strings[0].getChoices();
    return true;
  }
  else
    return false;
}

bool localSolverClient::buildRmCommand(std::string &cmd){
  std::vector<std::string> choices;

  if(getList("OutputFiles",choices)){
#if defined(WIN32)
    cmd.assign("del ");
#else
    cmd.assign("rm -rf ");
#endif
    for(unsigned int i = 0; i < choices.size(); i++)
      cmd.append(choices[i]+" ");
    return true;
  }
  else
    return false;
}

// METAMODEL

//Metamodel::analyze and Metamodel::compute are user defined (file: onelab.cpp)

void MetaModel::saveCommandLines(const std::string fileName){ //save client command lines
  std::string fileNameSave = fileName + onelabExtension + ".save";
  std::ofstream outfile(fileNameSave.c_str()); 

  if (outfile.is_open())
    for(citer it = _clients.begin(); it != _clients.end(); it++)
      outfile << (*it)->toChar();
  else
    Msg::Fatal("The file <%s> cannot be opened",fileNameSave.c_str());
  outfile.close();
}

bool MetaModel::checkCommandLines(){
  bool allDefined=true;
  for(citer it = _clients.begin(); it != _clients.end(); it++){
    allDefined = allDefined && (*it)->checkCommandLine();
  }
  saveCommandLines(genericNameFromArgs);
  return allDefined;
}

void MetaModel::initialize()
{
  Msg::Info("Initialize Metamodel by the loader");
  Msg::SetOnelabString(clientName + "/9CheckCommand","-a",false);
  Msg::SetOnelabNumber(clientName + "/UseCommandLine",1,false);
  Msg::SetOnelabNumber(clientName + "/Initialized",1,false);
}

void MetaModel::registerClient(const std::string &name, const std::string &type, const std::string &cmdl) {
  localSolverClient *c;
  if(!type.compare(0,6,"interf"))
    c= new InterfacedClient(name,cmdl);
  else if(!type.compare(0,6,"encaps"))
    c= new EncapsulatedClient(name,cmdl);
  else 
    Msg::Fatal("Unknown client type", type.c_str());
  _clients.push_back(c); 
}

void MetaModel::registerClient(const std::string &name, const std::string &type, const std::string &cmdl, const std::string &host, const std::string &dir) {
  localSolverClient *c;
  if(!type.compare(0,6,"interf"))
    c= new RemoteInterfacedClient(name,cmdl,host,dir);
  else if(!type.compare(0,6,"encaps"))
    c= new RemoteEncapsulatedClient(name,cmdl,host,dir);
  else 
    Msg::Fatal("Unknown remote client type", type.c_str());
  _clients.push_back(c); 
}

void MetaModel::simpleCheck()
{
  for(citer it = _clients.begin(); it != _clients.end(); it++){
    if((*it)->getActive()){
	Msg::SetOnelabString((*it)->getName() + "/Action","check",false);
	(*it)->analyze();
      }
  }
}

void MetaModel::simpleCompute()
{
  for(citer it = _clients.begin(); it != _clients.end(); it++){
    if((*it)->getActive()){
	Msg::SetOnelabString((*it)->getName() + "/Action","compute",false);
	(*it)->compute();
      }
  }
}

void MetaModel::PostArray(std::vector<std::string> choices)
{
  int nb=0;
  onelab::number o;
  while( 4*(nb+1) <= choices.size()){
    //std::cout << "Nb Choices" << choices.size() << std::endl;
    int lin= atof(choices[4*nb+1].c_str());
    int col= atof(choices[4*nb+2].c_str());
    double val=find_in_array(lin,col,read_array(choices[4*nb],' '));
    Msg::AddOnelabNumberChoice(choices[4*nb+3],val);
    Msg::Info("PostArray <%s>=%e",choices[4*nb+3].c_str(),val);
    nb++;
  }
}

// INTERFACED client

void InterfacedClient::analyze() {
  int pos;
  std::vector<std::string> choices;
  Msg::SetOnelabString(getName() + "/Action","check",false);// a titre indicatif
  getList("InputFiles", choices);
  for(unsigned int i = 0; i < choices.size(); i++){
    std::string ifilename = choices[i];
    if((pos=ifilename.find(onelabExtension)) != std::string::npos){
      checkIfPresent(ifilename);
      parse_onefile(ifilename); // recursive(?)
    }
  }
}

void InterfacedClient::convert() {
  int pos;
  std::vector<std::string> choices;
  getList("InputFiles", choices);
  for(unsigned int i = 0; i < choices.size(); i++){
    std::string ifilename = choices[i];
    checkIfPresent(ifilename);
    if((pos=ifilename.find(onelabExtension)) != std::string::npos){
      std::string ofilename = ifilename.substr(0,pos);  // remove .ol extension
      std::ofstream outfile(ofilename.c_str());
      if (outfile.is_open())
	convert_onefile(ifilename,outfile);
      else
	Msg::Fatal("The file <%s> cannot be opened",ofilename.c_str());
      outfile.close();
    }
  }
}

void InterfacedClient::compute(){
  std::string cmd;
  std::vector<std::string> choices;

  convert();
  Msg::SetOnelabString(getName() + "/Action","compute",false); // a titre indicatif

  if(getList("InputFiles",choices)){
    for(unsigned int i = 0; i < choices.size(); i++)
      checkIfPresent(choices[i].substr(0,choices[i].find(onelabExtension))); // remove .ol extension if any
  }

  if(buildRmCommand(cmd))
    SystemCall(cmd,true);

  cmd = FixWindowsPath(getCommandLine() + " ") ;
  cmd.append(getString("Arguments"));
  //commandLine.append(" &> " + _name + ".log");

  //printf("ONELAB: System call <%s>\n", cmd.c_str());
  //system(cmd.c_str());
  SystemCall(cmd.c_str(),true); //blocking

  if(getList("OutputFiles",choices)){
    for(unsigned int i = 0; i < choices.size(); i++)
      checkIfPresent(choices[i]);
  }

  Msg::Info("Client %s completed",_name.c_str());
}

// ENCAPSULATED Client

void EncapsulatedClient::analyze() {
  set(onelab::string(getName()+"/Action", "check"));
  run();
}

void EncapsulatedClient::compute() {
  std::string cmd;
  set(onelab::string(getName()+"/Action", "compute"));
  if(buildRmCommand(cmd))
    SystemCall(cmd,true);
  run();
}

// REMOTE CLIENT

int mySystem(std::string commandLine){
  //std::cout << "mySystem<" << commandLine << ">" << std::endl;
  return SystemCall(commandLine.c_str(), true);
}

bool remoteClient::checkIfPresentRemote(const std::string &fileName){
  struct stat buf;
  std::string cmd;
  char cbuf [1024];
  FILE *fp;

  cmd.assign("ssh "+_remoteHost+" 'cd "+_remoteDir+"; ls "+fileName+" 2>/dev/null'");
  std::cout << "check remote<" << cmd << ">" << std::endl;
  fp = popen(cmd.c_str(), "r");
  if(fgets(cbuf, 1024, fp) == NULL){
    std::cout << "le fichier " << fileName << " n'existe pas" << std::endl;
    pclose(fp);
    return false;
  }
  std::cout << "le fichier " << fileName << " existe" << std::endl;
  pclose(fp);
  return true;
}

bool remoteClient::syncInputFile(const std::string &fileName){
  int pos;
  std::string ofilename, cmd;
  if((pos=fileName.find(onelabExtension)) != std::string::npos){ // .ol file
    ofilename = fileName.substr(fileName.find_first_not_of(" "),pos);  // remove extension
    std::cout << "ofilename=<" << ofilename << ">" << std::endl;
    if(checkIfPresent(ofilename)){
      cmd.assign("rsync -e ssh -auv "+ofilename+" "+_remoteHost+":"+_remoteDir);
      return mySystem(cmd);
    }
    else{
      Msg::Fatal("The input file <%s> is not present present",ofilename.c_str());
      return false;
    }
  }
  else { // not a .ol file, apply the . rule
    if(!fileName.compare(fileName.find_first_not_of(" "),1,".")){ //should be found local and rsynced
      if(checkIfPresent(fileName)){
	cmd.assign("rsync -e ssh -auv "+fileName+" "+_remoteHost+":"+_remoteDir);
	return mySystem(cmd);
      }
      else{
	Msg::Fatal("The input file <%s> is not present present",fileName.c_str());
	return false;
      }
    }
    else { //should be found remote
      if(!checkIfPresentRemote(fileName)){
	Msg::Fatal("The input file <%s> is not present present",fileName.c_str());
	return false;
      }
      else
	return true;
    }
  }
}

bool remoteClient::syncOutputFile(const std::string &fileName){
  std::string cmd;

  if(checkIfPresentRemote(fileName)){
    int pos=fileName.find_first_not_of(" ");
    std::cout << "sync output" << std::endl;
    if(!fileName.compare(pos,1,".")){ // the file must be copied back locally
      cmd.assign("rsync -e ssh -auv "+_remoteHost+":"+_remoteDir+dirSep
		 +fileName.substr(pos,std::string::npos)+" .");
      return mySystem(cmd);
    }
  }
  else
    return false;
}


// REMOTE INTERFACED Client

bool RemoteInterfacedClient::checkCommandLine(){
  struct stat buf;
  std::string cmd;
  char cbuf [1024];
  FILE *fp;

  cmd.assign("ssh "+getRemoteHost()+" 'mkdir -p "+getRemoteDir()+"'");
  mySystem(cmd);

  cmd.assign("ssh "+getRemoteHost()+" 'which "+getCommandLine()+"'");
  fp = popen(cmd.c_str(), "r");
  if(fgets(cbuf, 1024, fp) == NULL){
    std::cout << "l'executable " << getCommandLine() << " n'existe pas" << std::endl;
    pclose(fp);
    return false;
  }
  std::cout << "l'executable " << getCommandLine() << " existe" << std::endl;
  pclose(fp);

  return true;
}


void RemoteInterfacedClient::compute(){
  std::string cmd,rmcmd;
  std::vector<std::string> choices;

  convert();
  Msg::SetOnelabString(getName() + "/Action","compute",false); // a titre indicatif

  if(getList("InputFiles",choices)){
    for(unsigned int i = 0; i < choices.size(); i++)
      syncInputFile(choices[i]);
  }

  if(buildRmCommand(rmcmd)){
    cmd.assign("ssh "+getRemoteHost()+" 'cd "+getRemoteDir()+"; "+rmcmd+"'");
    mySystem(cmd);
  }

  cmd.assign("ssh "+getRemoteHost()+" 'cd "+getRemoteDir()+"; "
	     +getCommandLine()+" "+getString("Arguments")+"'");
  mySystem(cmd);

  if(getList("OutputFiles",choices)){
    for(unsigned int i = 0; i < choices.size(); i++)
      syncOutputFile(choices[i]);
  }
}

// REMOTE ENCAPSULATED Client

std::string RemoteEncapsulatedClient::buildCommandLine(){
  std::vector<onelab::string> ps;
  get(ps, getName() + "/Action");
  std::string action = (ps.empty() ? "" : ps[0].getValue());
  get(ps, getName() + "/9CheckCommand");
  std::string checkCommand = (ps.empty() ? "" : ps[0].getValue());
  get(ps, getName() + "/9ComputeCommand");
  std::string computeCommand = (ps.empty() ? "" : ps[0].getValue());

  std::string cmd;
  cmd.assign("incomp_ssh "+getRemoteHost()+" 'cd "+getRemoteDir()+"; nohup "
	     +FixWindowsPath(getCommandLine())+" ");

  if(action == "initialize")
    cmd.append(" ");
  else if(action == "check")
    cmd.append(" " + getString("Arguments") + " " + checkCommand + " ");
  else if(action == "compute")
    cmd.append(" " + getString("Arguments") + " " + computeCommand + " ");
  else
    Msg::Fatal("remoteEncapsulatedClient::run: Unknown: Unknown Action <%s>", action.c_str());

   // append "-onelab" command line argument
  //cmd.append(getSocketSwitch() + " \"" + getName() + "\"");
  cmd.append(getSocketSwitch() + " " + getName() + " ");

  return cmd;
}

bool RemoteEncapsulatedClient::checkCommandLine(){
  struct stat buf;
  std::string cmd;
  char cbuf [1024];
  FILE *fp;

  cmd.assign("ssh "+getRemoteHost()+" 'mkdir -p "+getRemoteDir()+"'");
  mySystem(cmd);

  cmd.assign("ssh "+getRemoteHost()+" 'which "+getCommandLine()+"'");
  fp = popen(cmd.c_str(), "r");
  if(fgets(cbuf, 1024, fp) == NULL){
    std::cout << "l'executable " << getCommandLine() << " n'existe pas" << std::endl;
    pclose(fp);
    return false;
  }
  std::cout << "l'executable " << getCommandLine() << " existe" << std::endl;
  pclose(fp);

  Msg::SetOnelabString(getName() + "/Action","initialize",false);
  run();

  return true;
}


void RemoteEncapsulatedClient::compute(){
  std::string cmd,rmcmd;
  std::vector<std::string> choices;

  Msg::SetOnelabString(getName() + "/Action","compute",false); // a titre indicatif

  if(getList("InputFiles",choices)){
    for(unsigned int i = 0; i < choices.size(); i++)
      syncInputFile(choices[i]);
  }

  if(buildRmCommand(rmcmd)){
    cmd.assign("ssh "+getRemoteHost()+" 'cd "+getRemoteDir()+"; "+rmcmd+"'");
    mySystem(cmd);
  }

  run();

  if(getList("OutputFiles",choices)){
    for(unsigned int i = 0; i < choices.size(); i++)
      syncOutputFile(choices[i]);
  }
}

// ONELAB additional TOOLS (no access to server in tools)

// utilisé pour le main() des métamodèles
int getOptions(int argc, char *argv[], std::string &action, std::string &commandLine, std::string &fileName, std::string &clientName, std::string &sockName, int &modelNumber){

  commandLine=argv[0];
  action="compute";
  fileName="untitled";
  int i= 1;
  while(i < argc) {
    if(argv[i][0] == '-') {
      if(!strcmp(argv[i] + 1, "m")) {
	i++;
	modelNumber = (atoi(argv[i]));
        i++;
      }
      else if(!strcmp(argv[i] + 1, "onelab")) {
	i++;
	clientName = argv[i];
        i++;
	sockName = argv[i];
        i++;
      }
      else if(!strcmp(argv[i] + 1, "a")) {
	i++;
	action="check";
      }
      else if(!strcmp(argv[i] + 1, "c")) {
	i++;
	std::cout << "\nONELAB: Present state of the onelab clients\n" << std::endl;
	for(onelab::server::citer it = onelab::server::instance()->firstClient();
	    it != onelab::server::instance()->lastClient(); it++){
	  std::string name= it->second->getName();
	  std::cout << "<" << onelab::server::instance()->getChanged(name) << "> " << name << std::endl;
	}
	action="check";
      }
      else {
	i++;
	printf("Usage: %s [-m num -a -c]\n", argv[0]);
	printf("Options are:\nm      model number\n");
	printf("a      analyze only\n");
	exit(1);
      }
    }
    else{
      fileName=argv[i];
      i++;
    }
  }
}

static std::string getNextToken(const std::string &msg,
				std::string::size_type &first){
  if(first == std::string::npos) return "";
  std::string::size_type last = msg.find_first_of(charSep(), first);
  std::string next = msg.substr(first, last - first);
  first = (last == std::string::npos) ? last : last + 1;
  return next;
}
 
std::string itoa(const int i){
  std::ostringstream tmp;
  tmp << i ;
  return tmp.str();
}

#include <sys/stat.h>		
#include <ctime>
bool checkIfPresent(std::string fileName){
  struct stat buf;
  if (!stat(fileName.c_str(), &buf))
    return true;
  else{
    Msg::Info("The file %s is not present",fileName.c_str());
    return false;
  }
}

#include <unistd.h>
#include <sys/types.h>
#if not defined WIN32
#include <pwd.h>
std::string getUserHomedir(){
  struct passwd *pw = getpwuid(getuid());
  std::string str(pw->pw_dir);
  str.append("/");
  return str;
}
#endif

#include <sys/param.h>
std::string getCurrentWorkdir(){
  char path[MAXPATHLEN];
  getcwd(path, MAXPATHLEN);
  std::string str = path;
  return str;
}

std::string sanitize(const std::string &in)
{
  std::string out, forbidden(" ();");
  for(unsigned int i = 0; i < in.size(); i++)
    if ( forbidden.find(in[i]) == std::string::npos)
      out.push_back(in[i]);
  return out;
}
std::string removeBlanks(const std::string &in)
{
  int pos0=in.find_first_not_of(" ");
  int pos=in.find_last_not_of(" ");
  if( (pos0 != std::string::npos) && (pos != std::string::npos))
    return in.substr(pos0,pos-pos0+1);
  else
    return "";
}
int enclosed(const std::string &in, std::vector<std::string> &arguments){
  int pos, cursor;
  arguments.resize(0);
  cursor=0;
  if ( (pos=in.find("(",cursor)) == std::string::npos )
     Msg::Fatal("Syntax error: <%s>",in.c_str());

  unsigned int count=1;
  pos++; // skips '('
  cursor = pos; 
  do{
    if(in[pos]=='(') count++;
    if(in[pos]==')') count--;
    if(in[pos]==',') {
      arguments.push_back(removeBlanks(in.substr(cursor,pos-cursor)));
      if(count!=1)
	Msg::Fatal("Syntax error: <%s>",in.c_str());
      cursor=pos+1; // skips ','
    }
    pos++;
  } while( count && (pos!=std::string::npos) ); // find closing brace
  if(count)
     Msg::Fatal("Syntax error: <%s>",in.c_str());
  else
    arguments.push_back(removeBlanks(in.substr(cursor,pos-1-cursor)));
  return arguments.size();
}
int extract(const std::string &in, std::string &paramName, std::string &action, std::vector<std::string> &arguments){
  // syntax: paramName.action( arg1, arg2, ... )
  int pos, cursor,NumArg=0;
  cursor=0;
  if ( (pos=in.find(".",cursor)) == std::string::npos )
     Msg::Fatal("Syntax error: <%s>",in.c_str());
  else
    paramName.assign(sanitize(in.substr(cursor,pos-cursor)));
  cursor = pos+1; // skips '.'
  if ( (pos=in.find("(",cursor)) == std::string::npos )
     Msg::Fatal("Syntax error: <%s>",in.c_str());
  else
    action.assign(sanitize(in.substr(cursor,pos-cursor)));
  cursor = pos;
  unsigned int count=0;
  do{
    if(in[pos]=='(') count++;
    if(in[pos]==')') count--;
    pos++;
  } while(count && (pos!=std::string::npos) ); // find closing brace
  if(count)
     Msg::Fatal("Syntax error: %s",in.c_str());
  else
    NumArg = enclosed(in.substr(cursor,pos-cursor),arguments);
  // std::cout << "paramName=<" << paramName << ">" << std::endl;
  // std::cout << "arguments=<" << in.substr(cursor,pos+1-cursor) << ">" << std::endl;
  return NumArg;
}

bool extractRange(const std::string &in, std::vector<double> &arguments){
  // syntax: a:b:c or a:b#n
  int pos, cursor;
  arguments.resize(0);
  cursor=0;
  if ( (pos=in.find(":",cursor)) == std::string::npos )
     Msg::Fatal("Syntax error in range <%s>",in.c_str());
  else{
    arguments.push_back(atof(in.substr(cursor,pos-cursor).c_str()));
  }
  cursor = pos+1; // skips ':'
  if ( (pos=in.find(":",cursor)) != std::string::npos ){
    arguments.push_back(atof(in.substr(cursor,pos-cursor).c_str()));
    arguments.push_back(atof(in.substr(pos+1).c_str()));
  }
  else if ( (pos=in.find("#",cursor)) != std::string::npos ){
    arguments.push_back(atof(in.substr(cursor,pos-cursor).c_str()));
    double NumStep = atof(in.substr(pos+1).c_str());
    arguments.push_back((arguments[1]-arguments[0])/((NumStep==0)?1:NumStep));
  }
  else
     Msg::Fatal("Syntax error in range <%s>",in.c_str());
  return (arguments.size()==3);
}


void GmshDisplay(onelab::remoteNetworkClient *loader, std::string fileName, std::vector<std::string> choices){
  if(choices.empty()) return;

#if defined(WIN32)
  std::string cmd = "gmsh.exe ";
#else
  std::string cmd = "gmsh ";
#endif
  cmd.append( fileName + ".geo ");
  for(unsigned int i = 0; i < choices.size(); i++){
    cmd.append(choices[i]+" ");
    if(Msg::hasGmsh){
      loader->sendMergeFileRequest(choices[i]);
      Msg::Info("Send merge request <%s>",choices[i].c_str());
    }
  }
  if(!Msg::hasGmsh) system(cmd.c_str());
}


void appendOption(std::string &str, const std::string &what, const int val){
  if(val){ // assumes val=0 is the default
    std::stringstream Num;
    Num << val;
    str.append( what + " " + Num.str() + " ");
  }
}
void appendOption(std::string &str, const std::string &what){
  str.append( what + " ");
}

std::vector <double> extract_column(const int col, array data){
  std::vector<double> column;
  for ( int i=0; i<data.size(); i++)
    if (  col>0 && col<=data[i].size())
      column.push_back(data[i][col-1]);
    else
      Msg::Fatal("Column number (%d) out of range.",col);
  return column;
}

double find_in_array(const int lin, const int col, const std::vector <std::vector <double> > &data){
  if ( lin>=1 && lin<=data.size())
    if (  col>=1 && col<=data[lin-1].size())
      return data[lin-1][col-1];
  Msg::Fatal("The value has not been calculated: (%d,%d) out of range",lin,col);
}

array read_array(std::string filename, char sep){
  std::ifstream infile(sanitize(filename).c_str());
  std::vector <std::vector <double> > array;

  while (infile){
    std::string s;
    if (!getline( infile, s )) break;
    std::istringstream ss( s );
    std::vector <double> record;
    while (ss){
      std::string s;
      if (!getline( ss, s, sep )) break;
      if ( s.size() ){
	//std::cout << "Read=<" << s << ">" << std::endl;
	record.push_back( atof( s.c_str() ));
      }
    }
    array.push_back( record );
  }
  if (!infile.eof()){
    std::cerr << "Error reading array\n";
  }
  return array;
}

  
