# Linix OS is an operating system that support RISCV architecture. 

### main modules:
- Preemptive task scheduling
- File System
- Virtual Memory
- Network


### Run OS with QEMU emulator in Linux (Debian/Ubuntu)
install prerequsit tools:
```
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

run command:
```
make qemu
```


### Debug 
Run the command below to start gdb server:
```
make qemu-gdb
```
Then you can either use gdb cli or VScode as gdb frontend to connect to server for debugging.

#### gdb cli
Open a new terminal and run command to start gdb frontend:
```
gdb-multiarch
```


#### VSCode
1. Open project with VSCode
2. Install VSCode extension `Native Debug`
3. Run debug in VSCode as gdb frontend.

### FAQ
1. I want to build my own os but I don't know how to get started?
    Check out the `boot_minial` branch. It contains the minimum assembly code for OS boot and hand over the control to C code.

2. run into gdb debugging error?
    > warning:  ".gdbinit" auto-loading has been declined by your `auto-load safe-path' set to "$debugdir:$datadir/auto-load".

    create and open gdb config file `${home}/.config/gdb/gdbinit`. add the following line into the file:
    ```
    add-auto-load-safe-path ${project_root}/.gdbinit
    ```
