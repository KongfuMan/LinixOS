#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "file.h"
#include "stat.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  argint(n, &fd);
  if(fd < 0 || fd >= NOFILE || (f=current_proc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f){
    struct proc *p = current_proc();
    int i;
    for (i = 0; i < NFILE; i++){
        if (!p->ofile[i]){
            p->ofile[i] = f;
            return i;
        }
    }
    return -1;
}

// 1. load the binary (with elf format) into memory from disk by the file path
// 2. create a chid process to run the binary
uint64 
sys_exec(void){
    // arguments: a0, a1
    // - a0: path of binary image
    // - a1: arguments for binary
    char path[MAXPATH], *argv[MAXARG];
    int i;
    uint64 uargv, uarg;

    argaddr(1, &uargv);
    if(argstr(0, path, MAXPATH) < 0) {
        return -1;
    }

    memset(argv, 0, sizeof(argv)); 

    for(i=0;; i++){
        if(i >= NELEM(argv)){
            goto bad;
        }
        if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
            goto bad;
        }
        if(uarg == 0){
            argv[i] = 0;
            break;
        }
        argv[i] = kalloc();
        if(argv[i] == 0){
            goto bad;
        }
        if(fetchstr(uarg, argv[i], PGSIZE) < 0){
            goto bad;
        }
    }

    int ret = exec(path, argv);

    for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        kfree(argv[i]);

    return ret;

    bad:
        for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
            kfree(argv[i]);
        return -1;
}

static struct inode*
create(char *path, short type, short major, short minor){
    struct inode *ip, *dp;
    char name[DIRSIZE];

    if ((dp=nameiparent(path, name)) == 0){
        // parent dirent does not exist
        return 0;
    }

    ilock(dp);

    uint poff;
    if ((ip = dirlookup(dp, name, &poff)) != 0){
        // dirent already exist in parent dirent
        iunlockput(dp);
        ilock(ip); // ip = iget() already called in dirlookup
        if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE)){
            // dirent already exist, and type match, just return existing inode.
            return ip;
        }

        // dirent already exist, but type does not match.
        iunlockput(ip);
        return 0;
    }

    // create inode for `name`
    // create dirent (name, inum) and save it to parent dirent inode
    if ((ip = ialloc(dp->dev, type)) == 0){
        // allocate ip fail.
        iunlockput(dp);
        return 0;
    }

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip); // save the inode info

    // for a folder, create link "."->ip, ".."->dp
    if (type == T_DIR){
        if (dirlink(dp, ".", ip->inum) < 0 || dirlink(dp, "..", dp->inum) < 0){
            goto fail;
        }
    }

    if (dirlink(dp, name, ip->inum) < 0){
        goto fail;
    }

    // success
    if (type == T_DIR){
        dp->nlink++;
        iupdate(dp);
    }
    iunlockput(dp);
    return ip;

fail:
    // something went wrong. de-allocate ip.
    ip->nlink = 0;
    iupdate(ip);
    iunlockput(ip);
    iunlockput(dp);
    return 0;
}

// open a file by path with a MODE and return fd.
uint64
sys_open(void){
    char path[MAXPATH];
    int omode;
    struct inode *ip;
    struct file *f;
    int fd;
    if (argstr(0, path, MAXPATH) < 0){
        return -1;
    }
    argint(1, &omode);
    if ((ip = namei(path)) != 0){
        if (ip->type == T_DIR){
            return -1;
        }
        if ((omode & O_TRUNC) != 0){
            // free blocks
            // update inode
            ilock(ip);
            itrunc(ip);
            iunlock(ip);
        }
    }else if((omode & O_CREATE) == 0 || (ip = create(path, T_FILE, 0, 0)) == 0){
        return -1;
    }

    ilock(ip);
    // create file struct and add to proc->ofile
    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) == -1){
        if (f){
            fileclose(f);
        }
        iunlockput(ip);
        return -1;
    }

    // init fields of file
    if(ip->type == T_DEVICE){
        f->type = FD_DEVICE;
        f->major = ip->major;
    } else {
        f->type = FD_INODE;
        f->off = 0;
    }
    f->ip = ip;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    iunlock(ip);
    return fd;
}

uint64
sys_read(void){
    struct file *f;
    int n;
    uint64 p;

    argaddr(1, &p);
    argint(2, &n);
    if(argfd(0, 0, &f) < 0){
        return -1;
    }
    return fileread(f, p, n);
}

uint64
sys_write(void){
    struct file *f;
    int fd;
    int n;
    uint64 p;

    argaddr(1, &p);
    argint(2, &n);
    if(argfd(0, &fd, &f) < 0){
        return -1;
    }
    return filewrite(f, p, n);
}

uint64
sys_mknod(void){
    struct inode *ip;
    char path[MAXPATH];
    int major, minor;

    argint(1, &major);
    argint(2, &minor);

    if((argstr(0, path, MAXPATH)) < 0 ||
       (ip = create(path, T_DEVICE, major, minor)) == 0){
        return -1;
    }
    
    iunlockput(ip);
    return 0;
}

uint64
sys_dup(void){
    struct file *f;
    int fd;

    if(argfd(0, 0, &f) < 0){
        return -1;
    }
    if((fd=fdalloc(f)) < 0){
        return -1;
    }
    filedup(f);
    return fd;
}

uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  current_proc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

uint64
sys_socket(void){
    struct file *f;
    int fd;
    uint16 domain, type, protocol;
    argint(0, (int *)&domain);
    argint(1, (int *)&type);
    argint(2, (int *)&protocol);

    if(sockalloc(&f, domain, type, protocol) != 0){
        return -1;
    }
    if ((fd = fdalloc(f)) < 0){
        fileclose(f);
        return -1;
    }
    return fd;
}

uint64
sys_bind(void){
    // sockfd, src_ip, src_port
    // int bind(int, uint32, uint16);
    
    int fd;
    uint32 src_ip;
    uint16 src_port;

    struct file *f;
    if(argfd(0, &fd, &f) < 0){
        return -1;
    }

    argint(1, (int*)&src_ip);
    argint(2, (int*)&src_port);

    return sockbind(f->sock, src_ip, src_port);
}

uint64
sys_connect(void){
    struct file *f;
    int fd;
    int sockfd;
    uint32 dstip;
    uint32 srcport;
    uint32 dstport;
    argint(0, (int*)&sockfd);   
    argint(1, (int*)&dstip);
    argint(2, (int*)&srcport);
    argint(3, (int*)&dstport);

    if (sockconn(sockfd, dstip, srcport, dstport) < 0){
        return -1;
    }

    return 0;
}