/**********
© Copyright 2020 Xilinx, Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**********/

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "host.h"
#include "graph.cpp"
#include <unistd.h>
#include <complex>
#include <chrono>
#include <ctime>

#if !defined(__AIESIM__) && !defined(__X86SIM__)
#include "adf/adf_api/XRTConfig.h"
#include "xrt/experimental/xrt_device.h"
#include "xrt/experimental/xrt_kernel.h"
#include "xrt/experimental/xrt_bo.h"
#endif

#define CIN_LEN 25600
#define DIN_LEN 38400
#define OUT_LEN 38400

const int cin_size_in_bytes=CIN_LEN*8*8;
const int dlbf_din_size_in_bytes=DIN_LEN*8*4;
const int ulbf_din_size_in_bytes=DIN_LEN*8*8;
const int out_size_in_bytes=OUT_LEN*8*4;

class Timer {
  std::chrono::high_resolution_clock::time_point mTimeStart;
public:
  Timer() { reset(); }
  long long stop() {
    std::chrono::high_resolution_clock::time_point timeEnd =
        std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(timeEnd -
                                                                 mTimeStart)
        .count();
  }
  void reset() { mTimeStart = std::chrono::high_resolution_clock::now(); }
};

#if !defined(__AIESIM__) && !defined(__X86SIM__)
static std::vector<char>
load_xclbin(xrtDeviceHandle device, const std::string& fnm)
{
    if (fnm.empty())
        throw std::runtime_error("No xclbin speified");

    // load bit stream
    std::ifstream stream(fnm);
    stream.seekg(0,stream.end);
    size_t size = stream.tellg();
    stream.seekg(0,stream.beg);

    std::vector<char> header(size);
    stream.read(header.data(),size);

    auto top = reinterpret_cast<const axlf*>(header.data());
    if (xrtDeviceLoadXclbin(device, top))
        throw std::runtime_error("Bitstream download failed");

    return header;
}
#endif

int main(int argc, char* argv[]) {

    //TARGET_DEVICE macro needs to be passed from gcc command line
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] <<" <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    char* xclbinFilename = argv[1];
    
    // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // using customized allocator for getting buffer alignment to 4k boundary

#if !defined(__AIESIM__)
    // Load xclbin 
    std::cout << "Loading: '" << xclbinFilename << "'\n";
    auto dhdl = xrtDeviceOpen(0);
    auto xclbin = load_xclbin(dhdl, xclbinFilename);
    auto top = reinterpret_cast<const axlf*>(xclbin.data());
    adf::registerXRT(dhdl, top->m_header.uuid);
#endif

    xrtKernelHandle khdl_out[3];
    xrtKernelHandle khdl_cin[8];
    xrtKernelHandle khdl_din[2];

    xrtKernelHandle khdl_s2mm_v4_1 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "s2mm_v4:{s2mm_v4_1}");
    xrtKernelHandle khdl_s2mm_v4_2 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "s2mm_v4:{s2mm_v4_2}");
    xrtKernelHandle khdl_s2mm_v4_3 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "s2mm_v4:{s2mm_v4_3}");
    xrtKernelHandle khdl_mm2s_v4_1 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v4:{mm2s_v4_1}");
    xrtKernelHandle khdl_mm2s_v8_1 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_1}");
    xrtKernelHandle khdl_mm2s_v8_2 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_2}");
    xrtKernelHandle khdl_mm2s_v8_3 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_3}");
    xrtKernelHandle khdl_mm2s_v8_4 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_4}");
    xrtKernelHandle khdl_mm2s_v8_5 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_5}");
    xrtKernelHandle khdl_mm2s_v8_6 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_6}");
    xrtKernelHandle khdl_mm2s_v8_7 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_7}");
    xrtKernelHandle khdl_mm2s_v8_8 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_8}");
    xrtKernelHandle khdl_mm2s_v8_9 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s_v8:{mm2s_v8_9}");

    khdl_out[0]=khdl_s2mm_v4_1;
    khdl_out[1]=khdl_s2mm_v4_2;
    khdl_out[2]=khdl_s2mm_v4_3;
    khdl_cin[0]=khdl_mm2s_v8_1;
    khdl_cin[1]=khdl_mm2s_v8_2;
    khdl_cin[2]=khdl_mm2s_v8_3;
    khdl_cin[3]=khdl_mm2s_v8_4;
    khdl_cin[4]=khdl_mm2s_v8_6;
    khdl_cin[5]=khdl_mm2s_v8_7;
    khdl_cin[6]=khdl_mm2s_v8_8;
    khdl_cin[7]=khdl_mm2s_v8_9;
    khdl_din[0]=khdl_mm2s_v4_1;
    khdl_din[1]=khdl_mm2s_v8_5;

    short *host_out[3]; 
    short *host_cin[8]; 
    short *host_din[2];

    xrtBufferHandle out_bohdl[3];
    xrtBufferHandle cin_bohdl[8];
    xrtBufferHandle din_bohdl[2];
    for (int i = 0; i < 3; i++)
    {
        out_bohdl[i] = xrtBOAlloc(dhdl, out_size_in_bytes, 0, 0);
        host_out[i]  = reinterpret_cast<short*>(xrtBOMap(out_bohdl[i]));
    }
    for (int i = 0; i < 8; i++)
    {
        cin_bohdl[i] = xrtBOAlloc(dhdl, cin_size_in_bytes, 0, 0);
        host_cin[i]  = reinterpret_cast<short*>(xrtBOMap(cin_bohdl[i]));
    }
    din_bohdl[0] = xrtBOAlloc(dhdl, dlbf_din_size_in_bytes, 0, 0);
    host_din[0]  = reinterpret_cast<short*>(xrtBOMap(din_bohdl[0]));
    din_bohdl[1] = xrtBOAlloc(dhdl, ulbf_din_size_in_bytes, 0, 0);
    host_din[1]  = reinterpret_cast<short*>(xrtBOMap(din_bohdl[1]));

    // Manage/map data
    std::ifstream dlbf_din_files[4];
    std::ifstream dlbf_cin_files[32];
    std::ifstream ulbf_din_files[8];
    std::ifstream ulbf_cin_files[32];
    for(int i=0;i<4;i++){
	dlbf_din_files[i].open(std::string("data/dlbf_din")+char('0'+i)+".txt",std::ifstream::in);
    }
    for(int i=0;i<32;i++){
	dlbf_cin_files[i].open(std::string("data/dlbf_cin")+char('0'+i/4)+char('0'+i%4)+".txt",std::ifstream::in);
	ulbf_cin_files[i].open(std::string("data/ulbf_cin")+char('0'+i/8)+char('0'+i%8)+".txt",std::ifstream::in);
    }
    for(int i=0;i<8;i++){
	ulbf_din_files[i].open(std::string("data/ulbf_din")+char('0'+i)+".txt",std::ifstream::in);
    }
   
    for(int i=0;i<CIN_LEN;i++) {//Line number
        short d;
        for(int j=0;j<8;j++) {//CU number for dlbf & ulbf
            for(int k=0;k<8;k++) {//Stream number each CU
                for(int m=0;m<4;m++){//short number per line
                    if(j<4){//dlbf cin
                        dlbf_cin_files[8*j+k] >> std::dec >> d;
                    } else {//ulbf cin
                        ulbf_cin_files[8*(j-4)+k] >> std::dec >> d;
                    }
                    host_cin[j][i*32+k*4+m]=d;
                }
            }
        }
    }
    for(int i=0;i<DIN_LEN;i++){//Line number
        short d;
        for(int j=0;j<1;j++){//CU number for dlbf or ulbf
            for(int k=0;k<4;k++){//Stream number each dlbf CU
                for(int m=0;m<4;m++){//short number per line
                    dlbf_din_files[4*j+k] >> std::dec >> d;
                    host_din[0][i*16+k*4+m]=d;
                }
            }
            for(int k=0;k<8;k++){//Stream number each ulbf CU
                for(int m=0;m<4;m++){//short number per line
                    ulbf_din_files[8*j+k] >> std::dec >> d;
                    host_din[1][i*32+k*4+m]=d;
                }
            }
        }
    }

    // setup run handle
    xrtRunHandle rhdl_out[3];
    xrtRunHandle rhdl_cin[8];
    xrtRunHandle rhdl_din[2];

    int rval;
    for (int i = 0; i < 8; i++)
    {
        rhdl_cin[i] = xrtRunOpen(khdl_cin[i]);
        rval = xrtRunSetArg(rhdl_cin[i], 0, cin_bohdl[i]);
        rval = xrtRunSetArg(rhdl_cin[i], 9, CIN_LEN);
        rval = xrtRunStart(rhdl_cin[i]);
    }
    rhdl_din[0] = xrtRunOpen(khdl_din[0]);
    rval = xrtRunSetArg(rhdl_din[0], 0, din_bohdl[0]);
    rval = xrtRunSetArg(rhdl_din[0], 5, DIN_LEN);
    rval = xrtRunStart(rhdl_din[0]);

    rhdl_din[1] = xrtRunOpen(khdl_din[1]);
    rval = xrtRunSetArg(rhdl_din[1], 0, din_bohdl[1]);
    rval = xrtRunSetArg(rhdl_din[1], 9, DIN_LEN);
    rval = xrtRunStart(rhdl_din[1]);
    for (int i = 0; i < 3; i++)
    {
        rhdl_out[i] = xrtRunOpen(khdl_out[i]);
        rval = xrtRunSetArg(rhdl_out[i], 0, out_bohdl[i]);
        rval = xrtRunSetArg(rhdl_out[i], 5, OUT_LEN);
        rval = xrtRunStart(rhdl_out[i]);
    }
    std::cout << "rval:" << rval << std::endl;

    //Run graph
    std::cout<<"Starting graph run"<<std::endl;
    Timer timer;
    dut.run(100);
    dut.wait();

    double timer_stop=timer.stop();
    double throughput=OUT_LEN*2/timer_stop*1000000/1024/1024;
    std::cout<<"Throughput:"<<throughput<<"Msps"<<std::endl;
    std::cout<<"Graph run end"<<std::endl;

    dut.end();

    for (int i = 0; i < 8; i++)
    {
        auto state = xrtRunWait(rhdl_cin[i]);
        std::cout << "rhdl_cin[" << i << "] state:" << state << ")\n";
        xrtRunClose(rhdl_cin[i]);
        xrtKernelClose(khdl_cin[i]);
    }
    for (int i = 0; i < 2; i++)
    {
        auto state = xrtRunWait(rhdl_din[i]);
        std::cout << "rhdl_din[" << i << "] state:" << state << ")\n";
        xrtRunClose(rhdl_din[i]);
        xrtKernelClose(khdl_din[i]);
    }
    for (int i = 0; i < 3; i++)
    {
        auto state = xrtRunWait(rhdl_out[i]);
        std::cout << "rhdl_out[" << i << "] state:" << state << ")\n";
        xrtRunClose(rhdl_out[i]);
        xrtKernelClose(khdl_out[i]);
    }

    // get data to compare
    std::ofstream out_files[12];
    std::ifstream golden[12];
    for(int i=0;i<12;i++){
        if(i<8){
            out_files[i].open(std::string("data/dlbf_out")+char('0'+i)+".txt",std::ofstream::out);
            golden[i].open(std::string("data/dlbf_gold")+char('0'+i)+".txt",std::ifstream::in);
        }else{
            out_files[i].open(std::string("data/ulbf_out")+char('0'+i-8)+".txt",std::ofstream::out);
            golden[i].open(std::string("data/ulbf_gold")+char('0'+i-8)+".txt",std::ifstream::in);
        }
    }	
    int match = 0;
    for (int i = 0; i < OUT_LEN; i++) {//Line number
        short g;
        for(int j=0;j<3;j++){//CU number for dlbf or ulbf
            for(int k=0;k<4;k++){//Stream number per dlbf CU
                for(int m=0;m<4;m++){//Short number per line
                    golden[j*4+k] >> std::dec >> g;
                    if(g!=host_out[j][i*16+k*4+m]){
                        match=1;
                    }
                    out_files[j*4+k] << host_out[j][i*16+k*4+m] << " ";
                }
                out_files[j*4+k] << std::endl;
            }
        }
    }
    for(int i=0;i<12;i++){
        out_files[i].close();
        golden[i].close();
    }

    // clean up handles, BO,...
    for (int i = 0; i < 8; i++)
    {
        xrtBOFree(cin_bohdl[i]);
    }
    for (int i = 0; i < 2; i++)
    {
        xrtBOFree(din_bohdl[i]);
    }
    for (int i = 0; i < 3; i++)
    {
        xrtBOFree(out_bohdl[i]);
    }

    xrtDeviceClose(dhdl);

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
}

