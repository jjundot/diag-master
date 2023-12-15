# file directory 
* ./diag_master The UDS client module, which can be threaded and processable, relies on a third-party event driver library libev.  
* ./diag_master_api A series of apis are used to use the UDS client functionality.  
* ./diag_master_common File used by the UDS client and the UDS client API.  
* ./demo API Usage Examples.  
* ./lib_ev Tripartite library.  
# function Introduction
>It implements the management and execution of UDS request, response events and UDS serial tasks, and only needs to configure UDS tasks using apis，  
The UDS client module automatically executes UDS tasks and reports the results。  
>It can be used for vehicle OTA function development, remote diagnosis function, support doCAN,doIP and other parallel brush and parallel diagnosis  
# Integrated use
* **Use of the UDS client module**  
Copy the code in the diag_master and diag_master_common directories and rely on a libev to compile  
```
/* Call this function where the process is initialized */
diag_master_dms_start()
```
* **Use of the UDS client module API**  
Copy directory diag_master_api, diag_master_common code, no other dependencies can be directly compiled, directly call dm_api.h function  
* **Try using demo**  
```
/* Open a new terminal and compile the diag master module */
cd diag_master
make
/* Start the diag master module */
./diag_master
/* Open a new terminal and compile the diag master api library */
cd diag_master_api
make
/* Compile demo process */
cd demo
make
/* Run the demo process */
./01_demo
./02_demo
./03_demo
...
```
# Function example
* **01_demo.c**  
How do I use the UDS client API 
* **02_demo.c**  
How do I create a UDS client  
* **03_demo.c**  
How do I create a UDS Request service task  
* **04_demo.c**  
How to send UDS requests through doIP and doCAN    
* **05_demo.c**  
How do I create multiple UDS clients for parallel diagnosis requests    
* **06_demo.c**  
How can I obtain the response result of a UDS service task  
* **07_demo.c**  
How to configure the $27 service Security algorithm  
* **08_demo.c**  
How to create a DOIP client  
* **09_demo.c**  
How do I skip some request services  
* **10_demo.c**  
How to configure the $38 $36 $37 service  
* **11_demo.c**  
How to configure $34 $36 $37 service  
* **12_demo.c**  
How to configure the $27 service  
* **13_demo.c**  
How do I configure the service suppression response  
