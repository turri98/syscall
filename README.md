# syscall
Project of system call for Operating Systems for University of Verona


## How to build
For building on macOS:
- in semaphore.h in both directories clientExec and clientReq-server, the union must be called semunion

For building on linux:
- in semaphore.h in both directories clientExec and clientReq-server, the union must be called semun


Recommended way to build:
    You need 3 terminals I suggest: 1 for the server, 1 for the clientReq and 1 for clientExec

terminal 1)

```
cd /clientReq-server
make
```

terminal 2) 

```
cd /clientReq-server
```

terminal 3)

```
cd /clientExec
make
```


## How to run
* The terminal 1 is needed to control the correct execution of the entries, you don't need to write anything to it
* The terminal 2 is where you ask for the client keys for all activities, you need to type 
```./clientReq ``` and then you write the name and the service you want
* The terminal 3 is where you use the key terminal 2 gave you, you need to type  
``` ./clientExec user key some_arguments_depending_on_the_service ```

Arguments for terminal 3
* for STAMPA service: you can write anything, the service then will print out on the terminal your sentence
* for SALVA service: the first argument is the file name, after that, the words that need to be saved on the file
* for INVIA service: the first argument is the message queue id, after that, the words that need to be sent
