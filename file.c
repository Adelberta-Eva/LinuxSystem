#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include"const.h"



void my_create(char *filename) {
    fcb* fcbptr;
    fat* fat1, *fat2;
    char* fname, *exname, text[MAXTEXT];
    unsigned short blkno;
    int rbn, i;
    time_t now;
    struct tm* nowtime;

    fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat2 = (fat*)(myvhard + 3*BLOCKSIZE);

    fname = strtok(filename, ".");
    exname = strtok(NULL, ".");
    if (strcmp(fname, "") == 0) {
        printf("Error, creating file must have a right name.\n");
        return;
    }

    if (!exname) {
        printf("Error, creating file must have an external name.\n");
        return;
    }

    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++) {
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0) {
            printf("Error, the filename is already exist!\n");
            return;
        }
        fcbptr++;
    }

    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++) {
        if (fcbptr->free == 0)
            break;
        fcbptr++;
    }

    blkno = findblock();
    if (blkno == -1)
        return;
    (fat1 + blkno)->id = END;
    (fat2 + blkno)->id = END;

    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, fname);
    strcpy(fcbptr->exname, exname);
    fcbptr->attribute = 0x00;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 0;
    fcbptr->free = 1;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    fcbptr = (fcb*)text;
    fcbptr->length = openfilelist[curdir].length;
    openfilelist[curdir].count = 0;
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}


void my_rm(char *filename) {
    fcb* fcbptr;
    fat* fat1, *fat2, *fatptr1, *fatptr2;
    char* fname, *exname, text[MAXTEXT];
    unsigned short blkno;
    int rbn, i;

    fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);

    fname = strtok(filename, ".");
    exname = strtok(NULL, ".");

    if (strcmp(fname, "") == 0) {
        printf("Error, removing file must have a right name.\n");
        return;
    }

    if (!exname) {
        printf("Error, removing file must have an external name.\n");
        return;
    }

    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;

    for (i = 0; i < rbn / sizeof(fcb); i++) {
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break;
        fcbptr++;
    }

    if (i == rbn / sizeof(fcb)) {
        printf("Error, the file is not exist.\n");
        return;
    }

    blkno = fcbptr->first;
    while (blkno != END) {
        fatptr1 = fat1 + blkno;
        fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        fatptr1->id = FREE;
        fatptr2->id = FREE;
    }

    strcpy(fcbptr->filename, "");
    fcbptr->free = 0;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}


int my_open(char *filename) {
    fcb* fcbptr;
    char* fname, exname[3], *str, text[MAXTEXT];
    int rbn, fd, i;

    fname = strtok(filename, "."); // 将文件名和扩展名分离
    str = strtok(NULL, ".");
    if (str)
        strcpy(exname, str); // 如果存在扩展名，则将其复制到exname数组中
    else
        strcpy(exname, ""); // 如果不存在扩展名，则将空字符串复制到exname数组中
    for (i = 0; i < MAXOPENFILE; i++) { // 在用户打开文件数组中查找当前文件是否已经打开
        if (strcmp(openfilelist[i].filename, fname) == 0 && strcmp(openfilelist[i].exname, exname) == 0 && i != curdir) {
            printf("Error, the file is already open.\n"); // 如果文件已经打开，则打印错误信息并返回curdir
            return curdir;
        }
    }
    openfilelist[curdir].count = 0; // 重置当前目录的文件计数器
    rbn = do_read(curdir, openfilelist[curdir].length, text); // 读取当前目录下的文件列表
    exname[strlen(exname)-1]='\0';
    fcbptr = (fcb*)text; // 将文件列表转换为fcb指针类型
    for (i = 0; i < rbn / sizeof(fcb); i++) { // 在当前目录下查找要打开的文件是否存在
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break; // 如果找到了要打开的文件，则跳出循环
        fcbptr++; // 移动到下一个文件记录
    }

    if (i == rbn / sizeof(fcb)) { // 如果未找到要打开的文件
        printf("Error, the file does not exist.\n"); // 打印错误信息并返回curdir
        return curdir;
    }

    fd = findopenfile(); // 寻找空闲文件表项
    if (fd == -1)
        return curdir; // 如果找不到空闲文件表项，则返回curdir

    strcpy(openfilelist[fd].filename, fcbptr->filename); // 将找到的文件名复制到新打开文件的filename字段中
    strcpy(openfilelist[fd].exname, fcbptr->exname); // 将找到的扩展名复制到新打开文件的exname字段中
    openfilelist[fd].attribute = fcbptr->attribute; // 将找到的文件属性复制到新打开文件的attribute字段中
    openfilelist[fd].time = fcbptr->time; // 将找到的文件时间复制到新打开文件的time字段中
    openfilelist[fd].date = fcbptr->date; // 将找到的文件日期复制到新打开文件的date字段中
    openfilelist[fd].first = fcbptr->first; // 将找到的文件首部地址复制到新打开文件的first字段中
    openfilelist[fd].length = fcbptr->length; // 将找到的文件长度复制到新打开文件的length字段中
    openfilelist[fd].free = fcbptr->free; // 将找到的文件空闲标志复制到新打开文件的free字段中
    openfilelist[fd].dirno = openfilelist[curdir].first; // 将当前目录的首部地址复制到新打开文件的dirno字段中
    openfilelist[fd].diroff = i; // 将找到的文件在当前目录下的偏移量复制到新打开文件的diroff字段中
    strcpy(openfilelist[fd].dir, openfilelist[curdir].dir); // 将当前目录的路径复制到新打开文件的dir字段中
    strcat(openfilelist[fd].dir, filename); // 将文件名添加到新打开文件的dir字段中
    if (fcbptr->attribute & 0x20)
        strcat(openfilelist[fd].dir, "\\"); // 如果文件具有隐藏属性，则在dir字段后添加反斜杠
    openfilelist[fd].father = curdir; // 将当前目录的索引复制到新打开文件的father字段中
    openfilelist[fd].count = 0; // 将新打开文件的计数器初始化为0
    openfilelist[fd].fcbstate = 1; // 将新打开文件的状态设置为1
    openfilelist[fd].topenfile = 1; // 将新打开文件的topenfile字段设置为1
    return fd; // 返回新打开文件的索引
}


int my_close(int fd) {
    fcb *fcbptr;
    int father=-1;
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("Error, the file is not exist.");
        return -1;
    }
    if (openfilelist[fd].fcbstate) {
        fcbptr = (fcb*)malloc(sizeof(fcb));
        strcpy(fcbptr->filename, openfilelist[fd].filename);
        strcpy(fcbptr->exname, openfilelist[fd].exname);
        fcbptr->attribute = openfilelist[fd].attribute;
        fcbptr->time = openfilelist[fd].time;
        fcbptr->date = openfilelist[fd].date;
        fcbptr->first = openfilelist[fd].first;
        fcbptr->length = openfilelist[fd].length;
        fcbptr->free = openfilelist[fd].free;
        father = openfilelist[fd].father;
        openfilelist[father].count = openfilelist[fd].diroff * sizeof(fcb);
        do_write(father, (char*)fcbptr, sizeof(fcb), 2);
        free(fcbptr);
        openfilelist[fd].fcbstate = 0;
    }
    strcpy(openfilelist[fd].filename, "");
    strcpy(openfilelist[fd].exname, "");
    openfilelist[fd].topenfile = 0;
    return father;
}


int my_write(int fd) {
    fat *fat1, *fat2, *fatptr1, *fatptr2;
    int wstyle, len, ll, tmp;
    char text[MAXTEXT];
    unsigned short blkno;
    fat1 = (fat *)(myvhard + BLOCKSIZE);
    fat2 = (fat *)(myvhard + 3 * BLOCKSIZE);
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("The file is not exist!\n");
        return -1;
    }
    while (1) {
        printf("Please enter the number of write style:\n1. cut write\t2. cover write\t3. add write\n");
        scanf("%d", &wstyle);
        if (wstyle > 0 && wstyle < 4)
            break;
        printf("Input Error!");
    }
    getchar();
    switch (wstyle) {
    case 1: // 截断写把原文件所占的虚拟磁盘空间重置为1
        blkno = openfilelist[fd].first;
        fatptr1 = fat1 + blkno;
        fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        fatptr1->id = END;
        fatptr2->id = END;
        while (blkno != END) {
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
            blkno = fatptr1->id;
            fatptr1->id = FREE;
            fatptr2->id = FREE;
        }
        openfilelist[fd].count = 0;
        openfilelist[fd].length = 0;
        break;
    case 2:
        openfilelist[fd].count = 0;
        break;
    case 3:
        openfilelist[fd].count = openfilelist[fd].length;
        break;
    default:
        break;
    }
    ll = 0;
    printf("please input write data (end with Ctrl+D):\n");
    while (1) {
    	fgets(text,sizeof(text),stdin);
        len = strlen(text);
//	text[len-1]='\n';
        text[len] = '\0';
        tmp = do_write(fd, text, len, wstyle);
        if (tmp != -1)
            ll += tmp;
        if (tmp < len) {
            printf("Write Error!");
            break;
        }
	if(feof(stdin)){
	    clearerr(stdin);
	    break;
	}
    }
    printf("\n");
    return ll; // 实际写的字节数
}

int do_write(int fd, char *text, int len, char wstyle) {
    fat* fat1, *fat2, *fatptr1, *fatptr2;
    unsigned char* buf, *blkptr;
    unsigned short blkno, blkoff;
    int i, ll;
    fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
    buf = (unsigned char*)malloc(BLOCKSIZE);
    if (buf == NULL) {
        printf("malloc failed!\n");
        return -1;
    }
    blkno = openfilelist[fd].first;
    blkoff = openfilelist[fd].count;
    fatptr1 = fat1 + blkno;
    fatptr2 = fat2 + blkno;
    while (blkoff >= BLOCKSIZE) {
        blkno = fatptr1->id;
        if (blkno == END) {
            blkno = findblock();
            if (blkno == -1) {
                free(buf);
                return -1;
            }
            fatptr1->id = blkno;
            fatptr2->id = blkno;
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
            fatptr1->id = END;
            fatptr2->id = END;
        } else {
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
        }
        blkoff = blkoff - BLOCKSIZE; // 让blkoff定位到文件最后一个磁盘块的读写位置
    }

    ll = 0; // 实际写的字节数
    while (ll < len) // len是用户输入的字节数
    {
        blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE);
        for (i = 0; i < BLOCKSIZE; i++)
            buf[i] = blkptr[i];
        for (; blkoff < BLOCKSIZE; blkoff++) {
            buf[blkoff] = text[ll++];
            openfilelist[fd].count++;
            if (ll == len)
                break;
        }
        for (i = 0; i < BLOCKSIZE; i++)
            blkptr[i] = buf[i];
        if (ll < len) // 如果一个磁盘块写不下，则再分配一个磁盘块
        {
            blkno = fatptr1->id;
            if (blkno == END) {
                blkno = findblock();
                if (blkno == -1)
                    break;
                fatptr1->id = blkno;
                fatptr2->id = blkno;
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
                fatptr1->id = END;
                fatptr2->id = END;
            } else {
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
            }
            blkoff = 0;
        }
    }
    if (openfilelist[fd].count > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].count;
    openfilelist[fd].fcbstate = 1;
    free(buf);
    return ll; // 返回实际写入的字节数
}

int my_read(int fd, int len) {
    char text[MAXTEXT];
    int ll;
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("The file does not exist!\n");
        return -1;
    }
    openfilelist[fd].count = 0;
    ll = do_read(fd, len, text); // ll is the actual number of bytes read
    if (ll != -1) {
        printf("%s", text);
    } else {
        printf("ReadError!\n");
    }
    printf("\n");
    return ll;
}


int do_read(int fd, int len, char *text)
{
    fat *fat1, *fatptr;
    unsigned char *buf, *blkptr;
    unsigned short blkno, blkoff;
    int i, ll;

    fat1 = (fat *)(myvhard + BLOCKSIZE);
    buf = (unsigned char *)malloc(BLOCKSIZE);
    if (buf == NULL)
    {
        printf("mallocfailed!");
        return -1;
    }

    blkno = openfilelist[fd].first;
    blkoff = openfilelist[fd].count;
    if (blkoff > openfilelist[fd].length)
    {
        puts("Readoutofrange!");
        free(buf);
        return -1;
    }

    fatptr = fat1 + blkno;
    while (blkoff >= BLOCKSIZE) // blkoff为最后一块盘块剩余的容量
    {
        blkno = fatptr->id;
        blkoff = blkoff - BLOCKSIZE;
        fatptr = fat1 + blkno;
    }

    ll = 0;
    while (ll < len)
    {
        blkptr = (unsigned char *)(myvhard + blkno * BLOCKSIZE);
        for (i = 0; i < BLOCKSIZE; i++) // 将最后一块盘块的内容读取到buf中
            buf[i] = blkptr[i];
        for (; blkoff < BLOCKSIZE; blkoff++)
        {
            text[ll++] = buf[blkoff];
            openfilelist[fd].count++;
            if (ll == len || openfilelist[fd].count == openfilelist[fd].length)
                break;
        }
        if (ll < len && openfilelist[fd].count != openfilelist[fd].length)
        {
            blkno = fatptr->id;
            if (blkno == END)
                break;
            blkoff = 0;
            fatptr = fat1 + blkno;
        }
    }
    text[ll] = '\0';
    free(buf);
    return ll;
}

unsigned short findblock()
{
    unsigned short i;
    fat *fat1, *fatptr;
    fat1 = (fat *)(myvhard + BLOCKSIZE);
    for (i = 7; i < SIZE / BLOCKSIZE; i++)
    {
        fatptr = fat1 + i;
        if (fatptr->id == FREE)
            return i;
    }
    printf("Error, Can't find free block!");
    return -1;
}

int findopenfile()
{
    int i;
    for (i = 0; i < MAXTEXT; i++)
    {
        if (openfilelist[i].topenfile == 0)
            return i;
    }
    printf("Error, open too many files!");
    return -1;
}

