/*
 * gVirtuS -- A GPGPU transparent virtualization component.
 *
 * Copyright (C) 2009-2010  The University of Napoli Parthenope at Naples.
 *
 * This file is part of gVirtuS.
 *
 * gVirtuS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gVirtuS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gVirtuS; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Written by: Giuseppe Coviello <giuseppe.coviello@uniparthenope.it>,
 *             Department of Applied Science
 */

/**
 * @file   Process.cpp
 * @author Giuseppe Coviello <giuseppe.coviello@uniparthenope.it>
 * @date   Wed Sep 30 10:45:40 2009
 *
 * @brief
 *
 *
 */

#include "Process.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <dlfcn.h>

//addrmap module test
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "/home/gvirtus/addrmap/addrmap.h"
#include <fcntl.h>
#include <sys/syscall.h>

using namespace std;

static GetHandler_t LoadModule(const char *name) {
    char path[4096];
    if(*name == '/')
        strcpy(path, name);
    else
        sprintf(path, _PLUGINS_DIR "/lib%s-backend.so", name);

    void *lib = dlopen(path, RTLD_LAZY);
    if(lib == NULL) {
        cerr << "Error loading " << path << ": " << dlerror() << endl;
        return NULL;
    }

    HandlerInit_t init = (HandlerInit_t) ((uint64_t) dlsym(lib, "HandlerInit"));
    if(init == NULL) {
        dlclose(lib);
        cerr << "Error loading " << name << ": HandlerInit function not found."
                << endl;
        return NULL;
    }

    if(init() != 0) {
        dlclose(lib);
        cerr << "Error loading " << name << ": HandlerInit failed."
                << endl;
        return NULL;
    }

    GetHandler_t sym = (GetHandler_t) ((uint64_t) dlsym(lib, "GetHandler"));
    if(sym == NULL) {
        dlclose(lib);
        cerr << "Error loading " << name << ": " << dlerror() << endl;
        return NULL;
    }

    cout << "Loaded module '" << name << "'." << endl;

    return sym;
}

Process::Process(const Communicator *communicator, vector<string> &plugins)
: Subprocess(), Observable() {
    mpCommunicator = const_cast<Communicator *> (communicator);
    mPlugins = plugins;
}

Process::~Process() {
    cout << "[Process " << GetPid() << "]: Destroyed." << endl;
}

void Process::Setup() {

}

static bool getstring(Communicator *c, string & s) {
    s = "";
    char ch = 0;
    while(c->Read(&ch, 1) == 1) {
        if(ch == 0) {
            return true;
        }
        s += ch;
    }
    return false;
}

void Process::Execute(void * arg) {
    cout << "[Process " << GetPid() << "]: Started." << endl;

    //addrmap module test
	/*
    while(1)
    {
        int fd;
        fd = open("/home/gvirtus/addrmap/addrmap", O_RDWR);
        if(fd < 0)
        {
            printf("error open addrmap\n");
            break;
        }

        int addr = -1;
        ioctl(fd, IOCTL_ENVIAR_PID, getpid());
        addr = ioctl(fd, IOCTL_MAPADDR, 1);
        printf("addr: %x\n", addr);
        break;
    }

	*/

    GetHandler_t h;
    for(vector<string>::iterator i = mPlugins.begin(); i != mPlugins.end();
            i++) {
        if((h = LoadModule((*i).c_str())) != NULL)
            mHandlers.push_back(h());
    }
    Result * result;
    string routine;
    Buffer * input_buffer = new Buffer();
    while (getstring(mpCommunicator, routine)) {

	//whb
	//for kvm delay submission 
	//sleep this process
	
	if(routine == "cudaSetDeviceFlags") 
	{
		//syscall to sleep

		input_buffer->Reset(mpCommunicator); 

// copy 
	Handler *h = NULL;
        for(vector<Handler *>::iterator i = mHandlers.begin();
                i != mHandlers.end(); i++) {
            if((*i)->CanExecute(routine)) {
                h = *i;
                break;
            }
        }
        if(h == NULL) {
            cout << "[Process " << GetPid() << "]: Requested unknown routine "
                    << routine << "." << endl;
            result = new Result(-1, new Buffer());
        } else
        {
            result = h->Execute(routine, input_buffer);
		if(result->GetExitCode() == 0)
	    		result->setexitcode((int)GetPid());
		else
		{	
			cout << "kvm delay start error : cudaSetDeviceFlags return not 0" <<endl;
        		result->Dump(mpCommunicator);
			continue;
		}
        }

        result->Dump(mpCommunicator);

//end copy

	
		//when comes back, the delay submission has finished, then come back to while loop
		// get cudaMemcpy address in libcudart.so

		unsigned long* cuda_handlers = (unsigned long*) h->GetOriginalAddress_DEBUG();
		printf("DEBUG: memcpy addr: %u\n", (unsigned int)cuda_handlers);
		syscall(339, (unsigned long)cuda_handlers, cuda_handlers[0] * sizeof(unsigned long));
		printf("come back from syscall \n");
		continue;
	}
    //whb
    	//cout << "delay-start" <<endl;
    	if(routine == "cudaSetDevice")
	{
		int iter_counter = 0;
		/*
		int iters = input_buffer->Get<int>();
		cout << "iter_num:" << iters <<endl;
*/
        	input_buffer->Reset(mpCommunicator);

		while(!input_buffer->Empty())
		{
			iter_counter++;
			size_t tempbufsize = input_buffer->Get<size_t>();
			char* t = input_buffer->Get<char>(tempbufsize);
			Buffer* tempbuf = new Buffer(t,tempbufsize);

			char* MyRout;
			
			MyRout = tempbuf->AssignString();
			string MyRoutine(MyRout);
			//cout <<"myroutine:"<< MyRoutine << endl;
			
        Handler *h = NULL;
        for(vector<Handler *>::iterator i = mHandlers.begin();
                i != mHandlers.end(); i++) {
            if((*i)->CanExecute(routine)) {
                h = *i;
                break;
            }
        }
        if(h == NULL) {
            cout << "[Process " << GetPid() << "]: Requested unknown routine "
                    << routine << "." << endl;
            result = new Result(-1, new Buffer());
        } else
	{
            result = h->Execute(MyRoutine, tempbuf, true);
	}
       // result->Dump(mpCommunicator);
        if (result->GetExitCode() != 0) {
            cout << "[Process " << GetPid() << "]: Requested '" << routine
                    << "' routine." << endl;
            cout << "[Process " << GetPid() << "]: Exit Code '"
                    << result->GetExitCode() << "'." << endl;
        }

		}	//end of while
	result->Dump(mpCommunicator); // dump result to SetDevice
	delete result;
	
		continue;
	}		//end of delay
        input_buffer->Reset(mpCommunicator);
        Handler *h = NULL;
        for(vector<Handler *>::iterator i = mHandlers.begin();
                i != mHandlers.end(); i++) {
            if((*i)->CanExecute(routine)) {
                h = *i;
                break;
            }
        }
        if(h == NULL) {
            cout << "[Process " << GetPid() << "]: Requested unknown routine "
                    << routine << "." << endl;
            result = new Result(-1, new Buffer());
        } else
	{
		//whb
	    //cout << GetPid() << ": running routine:" << routine << endl;
            result = h->Execute(routine, input_buffer);
	}
        result->Dump(mpCommunicator);
        if (result->GetExitCode() != 0) {
            cout << "[Process " << GetPid() << "]: Requested '" << routine
                    << "' routine." << endl;
            cout << "[Process " << GetPid() << "]: Exit Code '"
                    << result->GetExitCode() << "'." << endl;
        }
        delete result;
    }
    delete input_buffer;
    Notify("process-ended");
    delete this;
}

