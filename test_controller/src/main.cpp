#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <signal.h>
#include <zmq.hpp>
#include <map>

#include "rpiMgr.h"
#include "utils.h"
#include "method.h"
#include "logger.h"

using namespace std;

const char connectionURL[] = "tcp://*:8000";  // The url for zeromq usage
RPiMgr *rpiMgr;
Logger *logger;

// Don't use SIGINT anymore, use multi thread instead

map<string, BaseMethod*> setup_method_map(zmq::socket_t& socket){
    map<string, BaseMethod*> method_map;
    method_map.insert(pair<string, BaseMethod*>("load", new Load(socket)));
    method_map.insert(pair<string, BaseMethod*>("play", new Play(socket)));
    method_map.insert(pair<string, BaseMethod*>("stop", new Stop(socket)));
    method_map.insert(pair<string, BaseMethod*>("statuslight", new StatusLight(socket)));
    method_map.insert(pair<string, BaseMethod*>("eltest", new Eltest(socket)));
    method_map.insert(pair<string, BaseMethod*>("ledtest", new Ledtest(socket)));
    method_map.insert(pair<string, BaseMethod*>("list", new List(socket)));
    method_map.insert(pair<string, BaseMethod*>("quit", new Quit(socket)));
    method_map.insert(pair<string, BaseMethod*>("pause", new Pause(socket)));
    return method_map;
}

int main(int argc, char *argv[]){
    if (argc != 2){
        cerr << "Error: missing parameters (dancerName)" << endl;
        exit(0);
    }

    bool quit = false;
    // Using zeromq
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind(connectionURL);

    string dancerName = argv[1];
    rpiMgr = new RPiMgr(dancerName);
    logger = new Logger(socket);

    // TODO: setDancer

    // Establish method mapping using strategy patter
    map<string, BaseMethod*> method_map = setup_method_map(socket);

    while (!quit){
        // Replace cin to zeromq socket.recv
        zmq::message_t request;

        // zmq server socket receive data
        socket.recv(&request);
        string inp = request.to_string();
        transform(inp.begin(), inp.end(), inp.begin(), ::tolower);
        vector<string> cmd = splitStr(inp);

        if (cmd.size() < 1)
            continue;
        
        if (method_map.count(cmd[0]) > 0)
            method_map[cmd[0]]->exec(cmd, quit);
        else {
            string err = "key not found \"" + cmd[0] + "\"\n";
            logger->error(err.c_str());
        }
    }
    return 0;
}