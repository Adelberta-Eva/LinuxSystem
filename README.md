# LinuxSystem
&nbsp;&nbsp;&nbsp;&nbsp;在虚拟机里面尝试写一个小的linux系统，主要功能包括创建、切换、删除目录，显示当前目录下的子目录，在当前文件下删除、创建、打开文件，在打开文件的状态下写入、读取文件信息，写入包括截断写、覆盖写和追加写。   
&nbsp;&nbsp;&nbsp;&nbsp;这是一次小组合作的系统写作，我主要写了my_ls()、my_write()、my_read()几个功能函数。但是在实际写的过程中，这种一个小团队共同协作完成一个小系统，不仅仅要明白自己的部分，还要明白其他部分的逻辑，比如一开始需要搭建的实际读取、写入do_ls()、do_write()、do_read（）,如果我的运行逻辑与底层逻辑不符，这个系统就不能运行。  
&nbsp;&nbsp;&nbsp;&nbsp;重难点主要集中在虚拟磁盘空间的合理布局与管理、文件读写操作的精确实现以及全面深入的系统测试。首先，需深入理解底层存储机制，合理规划磁盘空间以支持高效的文件与目录管理。其次，区别实现文件的截断写、覆盖写和追加写等复杂操作，需精确控制文件指针和缓存策略，确保数据一致性，在读写操作的时候，指针的运用尤其需要注意，测试时容易出现指针错误。最后，全面深入的系统测试是确保文件系统可靠性和稳定性的关键，需设计覆盖所有功能点的测试用例，并在测试过程中不断优化代码，提升系统性能。这些难点不仅考验了我们的技术能力，也加深了对文件系统设计与实现复杂性的理解。   
1.my_write()   
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;（1）截断写：清空原文件内容，释放原有 FAT 链表所占用的磁盘块，仅保留起始块   
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;（2）覆盖写：从文件起始位置开始覆盖写入，文件长度将按新写入内容更新    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;（3）追加写：从文件末尾开始写入，不影响原有内容，延续 FAT 链表追加新块    
2.do_write()    
&nbsp;&nbsp;&nbsp;&nbsp;此函数负责将输入数据写入虚拟磁盘，具体流程如下：    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;获取 FAT 表与文件起始块号    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;遍历 FAT 链表定位写入起始块和偏移位置    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;块内写入与缓冲区操作    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;将磁盘块内容读入缓冲区 buf    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;按写入模式将文本写入对应位置    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;若当前块写满，自动分配新块并更新 FAT 链   
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;写入完成后更新文件长度与 fcbstate 状态位    
3.my_read(int fd, int len)    
该函数调用 do_read() 从文件起始位置或当前位置开始读取指定长度数据，读取过程需注意：   
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;控制读取起始偏移，确保不会越界    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;按 FAT 链表遍历块，逐块读取并合并内容    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;支持读取过程中遇到文件尾时自动终止     
4.do_read(int fd, int len, char *text)    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;执行块级读取逻辑：     
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;获取当前块号与偏移量，检查是否越界    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;读取块内容至缓冲区 buf    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;将有效内容写入目标缓冲 text    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;若读取跨块，自动切换 FAT 链继续读取     
