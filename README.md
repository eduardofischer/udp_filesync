
# UDP File Synchronizer
File synchronizer project for the Operating Systems II class at [INF-UFRGS](http://www.inf.ufrgs.br/site/)

Developed by:
 - [Eduardo Fischer](https://github.com/eduardofischer/)
 - [Matheus Woeffel](https://github.com/MatheusWoeffel/)
 - [Rodrigo Bastos](https://github.com/ropbastos/)

## Dependencies
Before compiling, make sure you have all the dependencies installed

 - [readline.h](http://man7.org/linux/man-pages/man3/readline.3.html)

On Debian based platforms, like Ubuntu:
```
sudo apt-get install libreadline-dev 
```
If you use a platform with `yum`, like SUSE:
```
yum install readline-devel
```

## Build
Once you have installed all the dependencies, get the code:

```
git clone https://github.com/eduardofischer/udp_filesync.git
cd udp_filesync
```
To build the `client` and `server` binaries, run:
```
make
```
## Usage

Start the UDP FileSync `server` by running:
```
./server [-p PORT] [-b MAIN_SERVER_IP]
```
Then you can connect a `client` using:
```
./client <username> <server_ip_address> [<port>]
```
- When a client first connects to the server a directory named `sync_dir` is created in the client host.
- `sync_dir` will then be automatically synchronized among all the client instances connected to the server.

#### Client CLI
Once the client is connected to the server, a command line interface becomes available with the following options:

 `upload <filepath>`  - Uploads a file to the users shared directory
 
 `download <filename>`  - Downloads a file from the users shared directory
 
 `delete <filepath>`  - Deletes a file to the users local `sync_dir` directory
 
 `list_server`  - Lists all the files in the users shared directory
 
 `list_client`  - Lists all the files in the users local `sync_dir` directory
 
 `exit`  - Disconnects the client from the server