#include "kernel.h"
#include "fat16.h"

#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>


void startsys() {
    FILE *fp; // 文件类型的指针变量
    unsigned char buf[SIZE]; // 缓冲区数组
    fcb *root; // 文件控制块指针
    int i; // 循环计数器
    myvhard = (unsigned char*)malloc(SIZE); // 申请虚拟磁盘空间，用内存模拟磁盘空间
    memset(myvhard, 0, SIZE); // 将myvhard中前SIZE个字节用0替换并返回myvhard

    if ((fp = fopen(myfilename, "r")) != NULL) { // 转③，打开文件
        fread(buf, SIZE, 1, fp); // 将二进制文件读取到缓冲区
        fclose(fp);
        if (strcmp(((block0*)buf)->magic, "10101010")) { // 检查文件系统的魔数是否匹配
            printf("myfsys is not exist,begin to create the file...\n");
            my_format(); // 格式化文件系统
        } else {
            for (i = 0; i < SIZE; i++) {
                myvhard[i] = buf[i]; // 将缓冲区的内容复制到虚拟磁盘空间
            }
        }
    } else {
        printf("myfsys is not exist,begin to create the file...\n");
        my_format(); // 格式化文件系统
    }

    root = (fcb*)(myvhard + 5 * BLOCKSIZE); // 获取根目录的文件控制块地址
    strcpy(openfilelist[0].filename, root->filename); // 复制根目录的文件名到打开文件列表的第一个元素
    strcpy(openfilelist[0].exname, root->exname); // 复制根目录的扩展名到打开文件列表的第一个元素
    openfilelist[0].attribute = root->attribute; // 复制根目录的属性到打开文件列表的第一个元素
    openfilelist[0].time = root->time; // 复制根目录的时间到打开文件列表的第一个元素
    openfilelist[0].date = root->date; // 复制根目录的日期到打开文件列表的第一个元素
    openfilelist[0].first = root->first; // 复制根目录的首块号到打开文件列表的第一个元素
    openfilelist[0].length = root->length; // 复制根目录的长度到打开文件列表的第一个元素
    openfilelist[0].free = root->free; // 复制根目录的空闲块数到打开文件列表的第一个元素
    openfilelist[0].dirno = 5; // 设置根目录的目录编号为5
    openfilelist[0].diroff = 0; // 设置根目录的偏移量为0
    strcpy(openfilelist[0].dir, "\\root\\"); // 设置根目录的路径为"\\root\\"
    openfilelist[0].father = 0; // 设置根目录的父目录编号为0
    openfilelist[0].count = 0; // 设置根目录的子目录数量为0
    openfilelist[0].fcbstate = 0; // 设置根目录的状态为0
    openfilelist[0].topenfile = 1; // 设置根目录为已打开状态
    for (i = 1; i < MAXOPENFILE; i++) {
        openfilelist[i].topenfile = 0; // 将其他打开文件列表的元素设置为未打开状态
    }
    curdir = 0; // 设置当前目录编号为0
    strcpy(currentdir, "\\root\\"); // 设置当前目录路径为"\\root\\"
    startp = ((block0*)myvhard)->startblock; // 获取起始块号
}

void my_format() {
    FILE *fp;
    fat *fat1, *fat2;
    block0 *blk0;
    time_t now;
    struct tm *nowtime;
    fcb *root;
    int i;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
    root = (fcb*)(myvhard + 5 * BLOCKSIZE);

    strcpy(blk0->magic, "10101010");
    strcpy(blk0->information, "MyFileSystemVer1.0\nBlocksize=1KBWholesize=1000KBBlocknum=1000RootBlocknum=2\n");
    blk0->root = 5;
    blk0->startblock = (unsigned char*)root;

    for (i = 0; i < 5; i++) {
        fat1->id = END;
        fat2->id = END;
        fat1++;
        fat2++;
    }

    fat1->id = 6;
    fat2->id = 6;
    fat1++;
    fat2++;
    fat1->id = END;
    fat2->id = END;
    fat1++;
    fat2++;

    for (i = 7; i < SIZE / BLOCKSIZE; i++) {
        fat1->id = FREE;
        fat2->id = FREE;
        fat1++;
        fat2++;
    }

    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(root->filename, ".");
    strcpy(root->exname, "");
    root->attribute = 0x28;
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->first = 5;
    root->length = 2 * sizeof(fcb);
    root->free = 1;
    root++;

    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(root->filename, "..");
    strcpy(root->exname, "");
    root->attribute = 0x28;
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->first = 5;
    root->length = 2 * sizeof(fcb);
    root->free = 1;

    fp = fopen(myfilename, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
}



void my_exitsys()
{
    FILE *fp;
    while (curdir)
	curdir = my_close(curdir);
    fp = fopen(myfilename, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
    free(myvhard);
}
