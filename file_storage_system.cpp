#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <malloc.h>  
#include <signal.h>
#include <time.h>
#include<iostream>
#include <string>  
#include<windows.h>

#define BLOCKSIZE 32  
#define BLOCKNUM  32 
#define INODENUM  64  
#define FILENAME "file34.dat"  
using namespace std;


//超级块
typedef struct SuperBlock {
	unsigned short blockSize;
	unsigned short blockNum;
	unsigned short inodeNum;
	unsigned short blockFree;  		//剩余的bolck数 
	unsigned short inodeFree;  		//剩余的inode数 
	int blockBitmap[BLOCKNUM];  		//block数组 
	int inodeBitmap[INODENUM];  		//inode数组 
}SuperBlock;  //存储基本信息 

SuperBlock superBlock;  			//文件系统块信息
//INode 

typedef struct Inode {
	unsigned short inum;  			//inode的标号 
	char fileName[10];  			//文件名 
	unsigned short isDir;  			//inode对应的是文件还是目录：0 - file 1 - dir  
	unsigned short iparent;  		//该inode的父目录 
	unsigned short length;    		//if file->filesize  if dir->filenum  如果是文件，length为文件大小，如果是目录，length为该目录包含的文件个数 
	unsigned short blockNum;  		//Inode的block的个数 
}Inode, *PtrInode;

typedef struct Fcb {
	unsigned short inum;  					//文件id 
	char fileName[10];  					//文件名 
	unsigned short isDir;  					//目录 or 文件 
}Fcb, *PtrFcb;

unsigned short currentDir; 			//current inodenum  记录当前目录 
FILE *fp;
const unsigned short superBlockSize = sizeof(superBlock);
const unsigned short inodeSize = sizeof(Inode);
const unsigned short fcbSize = sizeof(Fcb);

char *argv[5]; 		//argv的每一个元素都是一个char数组 
int argc;
char path[80] = ""; 	//记录当前路径
HANDLE Mutex;//句柄

//查找当前目录下的文件 
int findInodeNum(char *name, int flag) {
	int i, fileInodeNum;
	PtrInode parentInode = (PtrInode)malloc(inodeSize);
	PtrFcb fcb = (PtrFcb)malloc(fcbSize);

	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	fread(parentInode, inodeSize, 1, fp);
	fseek(fp, superBlockSize + inodeSize * INODENUM + parentInode->blockNum * BLOCKSIZE, SEEK_SET);

	for (i = 0; i < parentInode->length; i++) {
		fread(fcb, fcbSize, 1, fp);
		if (flag == 0) {
			if ((fcb->isDir == 0) && (strcmp(name, fcb->fileName) == 0)) {
				fileInodeNum = fcb->inum;
				break;
			}
		}
		else {
			if ((fcb->isDir == 1) && (strcmp(name, fcb->fileName) == 0)) {
				fileInodeNum = fcb->inum;
				break;
			}
		}
	}

	if (i == parentInode->length)
		fileInodeNum = -1;

	free(fcb);
	free(parentInode);
	return fileInodeNum;
}

//分割命令 
int splitCommand(char* cmd) {
	const char *syscmd[] = { "cd", "dir", "copy", "del", "md", "more", "rd", "move", "help", "time", "ver", "rename", "touch", "write", "exit", "cls", "import", "export" };
	char temp[30];	//存最基础的命令 如 more，cd等 

	int i = 0;		//i为cmd的长度 
	char *ptr_temp;

	argc = 0;
	for (i = 0, ptr_temp = cmd; *ptr_temp != '\0'; ptr_temp++)
	{
		if (*ptr_temp != ' ')
		{
			while (*ptr_temp != ' ' && (*ptr_temp != '\0')) {
				temp[i] = *ptr_temp;
				i++;
				ptr_temp++;
			}
			argv[argc] = (char *)malloc(i + 1);
			//char *strncpy(char *dest, const char *src, int n)，把src所指向的字符串中以src地址开始的前n个字节复制到dest所指的数组中，并返回dest。 
			strncpy(argv[argc], temp, i);
			argv[argc][i] = '\0';
			//  
			//printf("%s\n",temp);
			//
			argc++;
			i = 0;
			if (*ptr_temp == '\0')
				break;
		}
	}

	if (argc != 0) {
		for (i = 0; (i < 18) && strcmp(argv[0], syscmd[i]); i++);
		return i;
	}
	else
		return 18;
	return 0;
}

//设置路径 
void setPath() {
	printf("root:%s\\>", path);
}




//切换目录 
void cd(char *name) {

	int fileInodeNum;
	PtrInode fileInode = (PtrInode)malloc(inodeSize);
	if (strcmp(name, "..") == 0) {
		fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		currentDir = fileInode->iparent;
		char temp[80] = "";
		int cnt = 0;
		for (int i = strlen(path) - 1; i >= 0; i--) {
			cnt++;
			if (path[i] == '\\')
				break;
		}
		strncpy(temp, path, strlen(path) - cnt);
		strcpy(path, temp);
	}
	else if (strcmp(name, "\\") == 0 || (strcmp(name, "\/") == 0)) {	//返回根目录
		fseek(fp, superBlockSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		currentDir = fileInode->iparent;
		strcpy(path, "");
	}
	else {
		char *subName;
		int cnt = 0;
		subName = strtok(name, "\\");
		while (subName != NULL) {
			fileInodeNum = findInodeNum(subName, 1);
			if (fileInodeNum == -1) {
				printf("目录不存在！\n", subName);
				break;
			}
			else {
				currentDir = fileInodeNum;
				strcat(path, "\\");
				strcat(path, subName);
			}
			subName = strtok(NULL, "\\");
		}
	}
	free(fileInode);
}

//查找节点
void searchInode(PtrInode &tempInode, int inum) {
	FILE *f = fopen(FILENAME, "r");
	fseek(f, superBlockSize + inodeSize * inum, SEEK_SET);
	fread(tempInode, inodeSize, 1, f);
	fclose(f);
}




//显示目录
void dir(char *config) {
	int dirCount = 0;
	int fileCount = 0;
	int CurTotalFileSize = 0;
	PtrFcb ptrFcb = (PtrFcb)malloc(fcbSize);
	PtrInode parentInode = (PtrInode)malloc(inodeSize);  		//指向当前目录 

	//parentInode指向当前目录的inode 
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	fread(parentInode, inodeSize, 1, fp);

	//指向当前目录的block 
	fseek(fp, superBlockSize + inodeSize * INODENUM + parentInode->blockNum * BLOCKSIZE, SEEK_SET);	

	if (strcmp(config, "") == 0) {
		for (int i = 0; i < parentInode->length; i++) {
			fread(ptrFcb, fcbSize, 1, fp);
			if (ptrFcb->isDir == 1) {				//是目录
				printf("<DIR>\t\t%s\t%d\n", ptrFcb->fileName, ptrFcb->inum);
				dirCount++;
			}
			else {									//是文件 
				PtrInode tempInode = (PtrInode)malloc(inodeSize);		//
				searchInode(tempInode, ptrFcb->inum);
				printf("<FIR>%d\t\t%s\n", tempInode->length, ptrFcb->fileName);
				CurTotalFileSize += tempInode->length;
				fileCount++;
			}
		}
	}
	else if (strcmp(config, "*.txt") == 0) {
		for (int i = 0; i < parentInode->length; i++) {
			fread(ptrFcb, fcbSize, 1, fp);
			if (ptrFcb->isDir == 0) {							//是文件 
				char sub[4];
				strncpy(sub, ptrFcb->fileName + (strlen(ptrFcb->fileName) - 4), 4);
				if (strcmp(sub, ".txt") == 0) {
					PtrInode tempInode = (PtrInode)malloc(inodeSize);		//记录当前Inode
					searchInode(tempInode, ptrFcb->inum);
					printf("%d\t\t%s\n", tempInode->length, ptrFcb->fileName);
					CurTotalFileSize += tempInode->length;
					fileCount++;
				}
			}
		}
	}
	printf("\t\t\t\t\t%d个文件\n", fileCount);
	printf("\t\t\t\t\t%d个目录\n", dirCount);


	free(ptrFcb);
	free(parentInode);
}

//创建文件或目录 
void createFile(char *name, int isDir) {			//flag = 0，创建文件 flag = 1，创建目录 
	//int nowBlockNum;		//记录要用的Block 
	//int nowInodeNum;  		//记录要用的Inode 
	//PtrInode nowInode = (PtrInode)malloc(inodeSize);
	//PtrInode parentInode = (PtrInode)malloc(inodeSize);
	//PtrFcb fcb = (PtrFcb)malloc(fcbSize);


	////查找可用的 block 
	//for (int i = 0; i < BLOCKNUM; i++) {
	//	if (superBlock.blockBitmap[i] == 0) {
	//		nowBlockNum = i;
	//		break;
	//	}
	//}

	////查找可用的 inode 
	//for (int i = 0; i < INODENUM; i++) {
	//	if (superBlock.inodeBitmap[i] == 0) {
	//		nowInodeNum = i;
	//		break;
	//	}
	//}

	////初始化该目录的inode 
	//nowInode->inum = nowInodeNum;
	//strcpy(nowInode->fileName, name);
	//if (isDir == 1)
	//	nowInode->isDir = 1;
	//else
	//	nowInode->isDir = 0;
	//nowInode->iparent = currentDir;
	//nowInode->length = 0;
	//nowInode->blockNum = nowBlockNum;

	////初始化fcb  
	//strcpy(fcb->fileName, nowInode->fileName);
	//fcb->inum = nowInode->inum;
	//fcb->isDir = nowInode->isDir;

	////把该目录的inode写到文件系统 
	//fseek(fp, superBlockSize + inodeSize * nowInodeNum, SEEK_SET);
	//fwrite(nowInode, inodeSize, 1, fp);
	//// cout<<"nowInode->inum:"<<nowInode->inum<<endl;

	// //更新 superBlock 和 bitmap  
	//superBlock.blockFree--;
	//superBlock.inodeFree--;
	//superBlock.blockBitmap[nowBlockNum] = 1;
	//superBlock.inodeBitmap[nowInodeNum] = 1;
	//rewind(fp);
	//fwrite(&superBlock, superBlockSize, 1, fp);		//写superBlock    

	////更新父目录的block 
	//fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//fread(parentInode, inodeSize, 1, fp);  			//parentInode指向父目录 
	////写当前文件的fcb 
	//fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE + parentInode->length * fcbSize, SEEK_SET);
	//fwrite(fcb, fcbSize, 1, fp);

	////更新父目录的inode 
	//parentInode->length++;
	//fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//fwrite(parentInode, inodeSize, 1, fp);

	//// free resource  
	//free(nowInode);
	//free(parentInode);
	//free(fcb);

	//fflush(fp);
	int index_block;
	int index_inode;
	for (int i = 0; i < BLOCKNUM; i++)
	{
		if (superBlock.blockBitmap[i] == 0)//找到一个空闲block块
		{
			index_block = i;
			superBlock.blockBitmap[i] = 1;//标记为已经占用
			break;
		}
	}
	for (int i = 0; i < INODENUM; i++)
	{
		if (superBlock.inodeBitmap[i] == 0)//找到一个空闲inode块
		{
			index_inode = i;
			superBlock.inodeBitmap[i] = 1;//标记为已经占用
			break;
		}
	}

	PtrInode nowinode = (PtrInode)malloc(sizeof(PtrInode));//新目录节点
	PtrInode fatherinode = (PtrInode)malloc(sizeof(PtrInode));//新父亲节点
	PtrFcb nowfcb = (PtrFcb)malloc(sizeof(PtrFcb));//新文件节点

	//更新nowinode节点
	nowinode->inum = index_inode;
	nowinode->blockNum = index_block;
	strcpy(nowinode->fileName, name);
	nowinode->iparent = currentDir;
	nowinode->isDir = isDir;
	nowinode->length = 0;

	//更新fcb节点
	nowfcb->isDir = isDir;
	strcpy(nowfcb->fileName, nowinode->fileName);
	nowfcb->inum = nowinode->inum;

	//更新superblock
	superBlock.blockFree--;
	superBlock.inodeFree--;
	
	//文件读写
	//写入nowinode
	fseek(fp, superBlockSize + inodeSize * index_inode, SEEK_SET);
	fwrite(nowinode, inodeSize, 1, fp);
	//写入superBlock
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//读出fatherinode节点
	fread(fatherinode, sizeof(Inode),1, fp);
	//将增加的文件写入
	fseek(fp, superBlockSize + INODENUM * inodeSize + fatherinode->blockNum * BLOCKSIZE + fatherinode->length * fcbSize, SEEK_SET);
	fwrite(nowfcb, fcbSize, 1, fp);
	//修改父目录
	fatherinode->length++;
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	fwrite(fatherinode, inodeSize, 1, fp);
	fflush(fp);
}

//创建目录 
void md(char *name) {
	createFile(name, 1);
}




//创建文件 
void touch(char *name) {
	createFile(name, 0);
}

//写文件
void write(char *name) {
	int fileInodeNum, cnt = 0;
	char c;
	PtrInode fileInode = (PtrInode)malloc(inodeSize);
	if ((fileInodeNum = findInodeNum(name, 1)) != -1) {
		printf("这是一个目录，不是一个文件\n");
		return;
	}

	fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1)
		printf("%s文件不存在\n", name);
	else {
		//找到文件的 inode 
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum * BLOCKSIZE, SEEK_SET);
		printf("请输入文件内容(以 # 号结尾):\n");
		while ((c = getchar()) != '#') {
			fputc(c, fp);
			cnt++;
		}
		c = getchar();

		//更新inode  
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fileInode->length = cnt;
		fseek(fp, -inodeSize, SEEK_CUR);
		fwrite(fileInode, inodeSize, 1, fp);
	}
	free(fileInode);
	fflush(fp);
}




//读文件
void more(char *name)
{
	char c;
	PtrInode fileInode = (PtrInode)malloc(inodeSize);
	int fileInodeNum = findInodeNum(name, 0);

	if (fileInodeNum == -1)
		printf("%s文件不存在\n", name);
	else {
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum * BLOCKSIZE, SEEK_SET);

		if (fileInode->length != 0)
		{
			for (int i = 0; i < fileInode->length; i++) {
				c = fgetc(fp);
				putchar(c);
			}
			printf("\n");
		}
	}
	free(fileInode);
}

//复制文件
void copy(char *source, char* path)
{
	//找到源文件
	int fileInodeNum = findInodeNum(source, 0);
	if (fileInodeNum == -1) {
		printf("找不到%s文件\n", source);
		return;
	}
	else {
		//操作源文件			
		PtrInode sourceInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(sourceInode, inodeSize, 1, fp);
		//cout << sourceInode->inum<<endl; 
		fseek(fp, superBlockSize + INODENUM * inodeSize + sourceInode->blockNum * BLOCKSIZE, SEEK_SET); 	//fp指向源文件的内容 

		char content[50];
		char ch;
		int i;
		for (i = 0; i < sourceInode->length; i++) {
			ch = fgetc(fp);
			content[i] = ch;
		}
		content[i] = '\0';

		//操作目标文件 
		cd(path);
		touch(source);			//在path路径下创建了一个source同名文件 
		PtrInode desInode = (PtrInode)malloc(inodeSize);		//desInode 		

		fseek(fp, superBlockSize + findInodeNum(source, 0) * inodeSize, SEEK_SET);
		fread(desInode, inodeSize, 1, fp);   						//desInode指向同名文件		
		fseek(fp, superBlockSize + INODENUM * inodeSize + desInode->blockNum * BLOCKSIZE, SEEK_SET);  //desFile指向同名文件的block

		for (i = 0; i < strlen(content); i++) {
			fputc(content[i], fp);
			fflush(fp);
		}

		//更新inode  
		fseek(fp, superBlockSize + findInodeNum(source, 0) * inodeSize, SEEK_SET);
		fread(desInode, inodeSize, 1, fp);
		desInode->length = strlen(content);
		fseek(fp, -inodeSize, SEEK_CUR);
		fwrite(desInode, inodeSize, 1, fp);
		char kk[3];
		strcpy(kk, "..");
		cd(kk);
	}
	fflush(fp);
}

//删除文件
void del(char *name)
{
	if (findInodeNum(name, 0) == -1 && findInodeNum(name, 1) == -1)//未找到指定文件或者目录
	{
		printf("未找到指定文件或者目录\n");
	}
	else
	{	
		//读取父目录
		int fileInodeNum, i;
		PtrInode fileInode = (PtrInode)malloc(inodeSize);
		PtrInode parentInode = (PtrInode)malloc(inodeSize);
		Fcb fcb[20];
		if ((fileInodeNum = findInodeNum(name, 0)) == -1)
			fileInodeNum = findInodeNum(name, 1);
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + fileInode->iparent * inodeSize, SEEK_SET);
		fread(parentInode, inodeSize, 1, fp);

		//删除父目录下所有fcb目录
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < parentInode->length; i++)
			fread(&fcb[i], fcbSize, 1, fp);  			//读取父目录下所有fcb 
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < BLOCKSIZE; i++)				//这个block全清空  
			fputc(0, fp);

		//恢复删除多了的文件
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < parentInode->length; i++) {
			if ((strcmp(fcb[i].fileName, name)) != 0)  //不是要删除的文件 
				fwrite(&fcb[i], fcbSize, 1, fp);  		//写进block里 
		}

		//更新父目录的inode信息 
		parentInode->length--;
		fseek(fp, superBlockSize + fileInode->iparent * inodeSize, SEEK_SET);
		fwrite(parentInode, inodeSize, 1, fp);

		//更新superBlock
		superBlock.inodeBitmap[fileInodeNum] = 0;
		superBlock.blockBitmap[fileInode->blockNum] = 0;
		superBlock.blockFree++;
		superBlock.inodeFree++;
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);		//写superBlock
		free(fileInode);
		free(parentInode);
		fflush(fp);       
	}
	
}


//重命名文件
void rename(char *originalName, char *newName)
{
	if ((findInodeNum(originalName, 0) == -1) && (findInodeNum(originalName, 1) == -1))
	{
		printf("未查找到指定文件或者目录\n");
	}
	else
	{
		int fileInodeNum, i;
		PtrInode fileInode = (PtrInode)malloc(inodeSize);
		PtrInode parentInode = (PtrInode)malloc(inodeSize);
		Fcb fcb[20];
		PtrFcb ptrFcb = (PtrFcb)malloc(fcbSize);
		if ((fileInodeNum = findInodeNum(originalName, 0)) == -1)
			fileInodeNum = findInodeNum(originalName, 1);

		//读取父目录的fcb
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + fileInode->iparent * inodeSize, SEEK_SET);
		fread(parentInode, inodeSize, 1, fp);

		//读取父目录下所有fcb
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < parentInode->length; i++) {
			fread(&fcb[i], fcbSize, 1, fp);  			
			if (fcb[i].inum == fileInodeNum)
				break;
		}

		//将原文件重命名
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE + i * fcbSize, SEEK_SET); 	//指向那个fcb
		fread(ptrFcb, fcbSize, 1, fp);
		memset(ptrFcb->fileName, '\0', sizeof(ptrFcb->fileName));
		strcpy(ptrFcb->fileName, newName);
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE + i * fcbSize, SEEK_SET); 	//指向那个fcb
		fwrite(ptrFcb, fcbSize, 1, fp);
		fflush(fp);
	}
}


//删除文件夹
void rd(char *name)
{
	int fileInodeNum = findInodeNum(name, 0);
	int dirInodeNum = findInodeNum(name, 1);
	if (fileInodeNum == -1 && dirInodeNum == -1) {
		printf("删除失败，请输入正确的文件名！\n");
		return;
	}
	if (fileInodeNum != -1 && dirInodeNum == -1) { 		//是文件 	
		//直接删除 
		del(name);
		return;
	}
	else if (dirInodeNum != -1 && fileInodeNum == -1) {			//是目录	 
		PtrInode ptrInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + dirInodeNum * inodeSize, SEEK_SET);
		fread(ptrInode, inodeSize, 1, fp);  		//ptrInode指向当前目录			
		if (ptrInode->length != 0) {				//该目录下还有文件
			cd(ptrInode->fileName);
			PtrFcb fcb = (PtrFcb)malloc(fcbSize);
			fseek(fp, superBlockSize + INODENUM * inodeSize + ptrInode->blockNum * BLOCKSIZE, SEEK_SET);

			for (int i = 0; i < ptrInode->length; i++) {
				fseek(fp, superBlockSize + INODENUM * inodeSize + ptrInode->blockNum * BLOCKSIZE, SEEK_SET);
				fread(fcb, fcbSize, 1, fp);  			//读取目录下所有fcb 
				rd(fcb->fileName);
			}
			char tt[3];
			strcpy(tt, "..");
			cd(tt);
			del(name);
			//cout << name<<endl;
			return;
		}
		else {
			del(name);
			return;
		}
	}
}


//export 导出文件到本地
void export_(char *name, char *path) {
	FILE *ft;
	path = strcat(path, name);
	if ((ft = fopen(path, "w")) != NULL) {
		int fileInodeNum = findInodeNum(name, 0);
		if (fileInodeNum == -1) {
			printf("%s文件不存在！", name);
			return;
		}
		PtrInode fileInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum * BLOCKSIZE, SEEK_SET);
		char ch;
		for (int i = 0; i < fileInode->length; i++) {
			ch = fgetc(fp);
			fputc(ch, ft);
			fflush(ft);
		}
	}
	else {
		printf("打开文件失败！\n");
	}
	fclose(ft);
}


//inport 将本地文件导入
void import_(char* name) {
	FILE *ftemp;

	if ((ftemp = fopen(name, "r+")) != NULL) {
		int cnt = 0;
		for (int i = strlen(name) - 1; i >= 0; i--) {
			if (name[i] == '\\')
				break;
			cnt++;
		}
		char filename[10];
		strncpy(filename, name + (strlen(name) - cnt), cnt);
		filename[cnt] = '\0';
		//cout << filename<<endl;

		touch(filename);			//在当前路径下创建同名文件 

		PtrInode fileInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + findInodeNum(filename, 0) * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);   						//desInode指向同名文件		
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum * BLOCKSIZE, SEEK_SET);  //指向同名文件的block

		int cntNum = 0;
		char c;
		while ((c = fgetc(ftemp)) != EOF) {			//写到block里去 
			fputc(c, fp);
			fflush(fp);
			cntNum++;
		}

		//更新inode  
		fseek(fp, superBlockSize + findInodeNum(filename, 0) * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fileInode->length = cntNum;
		fseek(fp, -inodeSize, SEEK_CUR);
		fwrite(fileInode, inodeSize, 1, fp);
		fflush(fp);
	}
	else {
		printf("打开文件失败！\n");
	}
	fclose(ftemp);

}


//移动文件
void move(char *source, char *des) {
	int sourceInodeNum = findInodeNum(source, 0);
	int desInodeNum = findInodeNum(des, 1);
	if (sourceInodeNum == -1) {
		printf("%s文件不存在！", source);
		return;
	}
	if (desInodeNum == -1) {
		printf("路径错误！");
		return;
	}
	copy(source, des);
	del(source);
}


//显示菜单
void help() {
	cout << "===========================文件系统=================================\n";
	cout << "   cd [Path]                        目录切换\n";
	cout << "   dir                              显示目录内容\n";
	cout << "   copy [FileName] [DirPath]        复制文件\n";
	cout << "   del [FileName]                   删除文件\n";
	cout << "   md [DirName]                     创建目录\n";
	cout << "   more [FileName]                  查看文件\n";
	cout << "   rd [DirName]                     删除目录\n";
	cout << "   move [FileName] [DirName]        转移文件\n";
	cout << "   help                             显示帮助信息\n";
	cout << "   rename [OldName] [NewName]       文件或目录重命名\n";
	cout << "   touch [FileName]                 创建文件\n";
	cout << "   write [FileName]                 写文件\n";
	cout << "   exit                             退出\n";
	cout << "   cls                              清屏\n";
	cout << "   import [Path]                    从本地磁盘复制文件到当前目录\n";
	cout << "   export [FileName] [Path]         将当前目录下的文件导出到本地磁盘\n";
	cout << "====================================================================\n";
}


//const char *syscmd[] = {"cd", "dir", "copy", "del", "md", "more", "rd", "move", "help", "time", "ver", "rename", "touch", "write", "exit", "cls", "import", "export"};
void command() {
	char cmd[30];
	do
	{
		WaitForSingleObject(Mutex, INFINITE);//只能有一个线程工作
		//FILE* f = fopen(FILENAME, "rb+");
		setPath();
		cin.getline(cmd,1024);
		for (int i = 0; i < 5; i++)
		{
				argv[i] = (char *)malloc(1024);//为指令开辟内存
				argv[i][0] = '\0';//赋初始值
		}
		switch (splitCommand(cmd)) {
		case 0:  		//cd
			cd(argv[1]);			//cd dira：argv[1]是dira 
			break;
		case 1:			//dir
			dir(argv[1]);
			break;
		case 2:  		//copy
			copy(argv[1], argv[2]);
			break;
		case 3:  		//del
			del(argv[1]);
			break;
		case 4:   		//md
			md(argv[1]);
			break;
		case 5:  		//more
			more(argv[1]);
			break;
		case 6:  		//rd：删除文件夹 
			rd(argv[1]);
			break;
		case 7:  		//move
			move(argv[1], argv[2]);
			break;
		case 8:  		//help
			help();
			break;
		case 9: 		
			break;
		case 10:  		
			break;
		case 11:  		//rename
			rename(argv[1], argv[2]);
			break;
		case 12:		//touch
			touch(argv[1]);
			break;
		case 13:		//write
			write(argv[1]);
			break;
		case 14:		//exit
			exit(0);
			ReleaseMutex(Mutex);//释放线程
			break;
		case 15:		//cls
			system("cls");
			break;
		case 16:
			import_(argv[1]);
			break;
		case 17:
			export_(argv[1], argv[2]);
			break;
		default:
			printf("命令错误\n");
			ReleaseMutex(Mutex);//释放线程
			break;
		}
	} while (1);
}




//创建文件系统 
void createFileSystem() {
	PtrInode ptrInode;
	if ((fp = fopen(FILENAME, "wb+")) == NULL) {  			//wb+是以二进制方式进行读写
		printf("打开文件失败！\n", FILENAME);
		exit(1);
	}

	//初始化 bitmap  
	for (int i = 0; i < BLOCKNUM; i++)
		superBlock.blockBitmap[i] = 0;				//未使用  

	for (int i = 0; i < INODENUM; i++)
		superBlock.inodeBitmap[i] = 0;  				//未使用 


	for (int i = 0; i < (superBlockSize + inodeSize * INODENUM + BLOCKSIZE * BLOCKNUM); i++)
		fputc(0, fp);  //将字符0写到文件指针fp所指向的文件的当前写指针的位置 
	rewind(fp); 			//使文件位置指针重新定位到fp文件的开始位置。 

	//初始化superBlock  
	superBlock.blockNum = BLOCKNUM;
	superBlock.blockSize = BLOCKSIZE;
	superBlock.inodeNum = INODENUM;
	superBlock.blockFree = BLOCKNUM - 1;
	superBlock.inodeFree = INODENUM - 1;

	//0号inode是根目录 
	//创建根目录  
	ptrInode = (PtrInode)malloc(inodeSize);
	ptrInode->inum = 0;
	strcpy(ptrInode->fileName, "/");  //根节点 
	ptrInode->isDir = 1;  			   //目录 
	ptrInode->iparent = 0;
	ptrInode->length = 0;
	ptrInode->blockNum = 0;

	//根目录已用 
	superBlock.inodeBitmap[0] = 1;
	superBlock.blockBitmap[0] = 1;

	//写superBlock 
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);		//写superBlock      

	//写根目录
	fseek(fp, superBlockSize, SEEK_SET);
	fwrite(ptrInode, inodeSize, 1, fp);

	//当前目录初始化为根目录 
	currentDir = 0;
	fflush(fp);
}






//打开文件系统 
void openFileSystem()
/*如果FILENAME可读，则代表之前已有信息，并读取相应数据    如果不可读，则创建文件系统 */
{
	if ((fp = fopen(FILENAME, "rb+")) != NULL) {			//存在文件系统 
		if ((fp = fopen(FILENAME, "rb+")) == NULL) {  	//rb+ 可读可写方式，打开一个二进制文件，不存在会报错 
			printf("打开文件失败！\n", FILENAME);
			exit(1);
		}
		rewind(fp);  		//使文件位置指针重新定位到fp文件的开始位置。

		//读超级块 
		fread(&superBlock, superBlockSize, 1, fp);
		//初始化当前目录 
		currentDir = 0;			//当前目录为根目录  	
	}
	else											//不存在文件系统 
		createFileSystem();
}






int main() {
	//signal(SIGINT, stopHandle);
	//	login();   
	Mutex = CreateMutex(NULL, FALSE, NULL);//创建线程
	openFileSystem();  			//打开文件系统 
	help();
	command();
	return 0;
}
































