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


//������
typedef struct SuperBlock {
	unsigned short blockSize;
	unsigned short blockNum;
	unsigned short inodeNum;
	unsigned short blockFree;  		//ʣ���bolck�� 
	unsigned short inodeFree;  		//ʣ���inode�� 
	int blockBitmap[BLOCKNUM];  		//block���� 
	int inodeBitmap[INODENUM];  		//inode���� 
}SuperBlock;  //�洢������Ϣ 

SuperBlock superBlock;  			//�ļ�ϵͳ����Ϣ
//INode 

typedef struct Inode {
	unsigned short inum;  			//inode�ı�� 
	char fileName[10];  			//�ļ��� 
	unsigned short isDir;  			//inode��Ӧ�����ļ�����Ŀ¼��0 - file 1 - dir  
	unsigned short iparent;  		//��inode�ĸ�Ŀ¼ 
	unsigned short length;    		//if file->filesize  if dir->filenum  ������ļ���lengthΪ�ļ���С�������Ŀ¼��lengthΪ��Ŀ¼�������ļ����� 
	unsigned short blockNum;  		//Inode��block�ĸ��� 
}Inode, *PtrInode;

typedef struct Fcb {
	unsigned short inum;  					//�ļ�id 
	char fileName[10];  					//�ļ��� 
	unsigned short isDir;  					//Ŀ¼ or �ļ� 
}Fcb, *PtrFcb;

unsigned short currentDir; 			//current inodenum  ��¼��ǰĿ¼ 
FILE *fp;
const unsigned short superBlockSize = sizeof(superBlock);
const unsigned short inodeSize = sizeof(Inode);
const unsigned short fcbSize = sizeof(Fcb);

char *argv[5]; 		//argv��ÿһ��Ԫ�ض���һ��char���� 
int argc;
char path[80] = ""; 	//��¼��ǰ·��
HANDLE Mutex;//���

//���ҵ�ǰĿ¼�µ��ļ� 
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

//�ָ����� 
int splitCommand(char* cmd) {
	const char *syscmd[] = { "cd", "dir", "copy", "del", "md", "more", "rd", "move", "help", "time", "ver", "rename", "touch", "write", "exit", "cls", "import", "export" };
	char temp[30];	//������������� �� more��cd�� 

	int i = 0;		//iΪcmd�ĳ��� 
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
			//char *strncpy(char *dest, const char *src, int n)����src��ָ����ַ�������src��ַ��ʼ��ǰn���ֽڸ��Ƶ�dest��ָ�������У�������dest�� 
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

//����·�� 
void setPath() {
	printf("root:%s\\>", path);
}




//�л�Ŀ¼ 
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
	else if (strcmp(name, "\\") == 0 || (strcmp(name, "\/") == 0)) {	//���ظ�Ŀ¼
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
				printf("Ŀ¼�����ڣ�\n", subName);
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

//���ҽڵ�
void searchInode(PtrInode &tempInode, int inum) {
	FILE *f = fopen(FILENAME, "r");
	fseek(f, superBlockSize + inodeSize * inum, SEEK_SET);
	fread(tempInode, inodeSize, 1, f);
	fclose(f);
}




//��ʾĿ¼
void dir(char *config) {
	int dirCount = 0;
	int fileCount = 0;
	int CurTotalFileSize = 0;
	PtrFcb ptrFcb = (PtrFcb)malloc(fcbSize);
	PtrInode parentInode = (PtrInode)malloc(inodeSize);  		//ָ��ǰĿ¼ 

	//parentInodeָ��ǰĿ¼��inode 
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	fread(parentInode, inodeSize, 1, fp);

	//ָ��ǰĿ¼��block 
	fseek(fp, superBlockSize + inodeSize * INODENUM + parentInode->blockNum * BLOCKSIZE, SEEK_SET);	

	if (strcmp(config, "") == 0) {
		for (int i = 0; i < parentInode->length; i++) {
			fread(ptrFcb, fcbSize, 1, fp);
			if (ptrFcb->isDir == 1) {				//��Ŀ¼
				printf("<DIR>\t\t%s\t%d\n", ptrFcb->fileName, ptrFcb->inum);
				dirCount++;
			}
			else {									//���ļ� 
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
			if (ptrFcb->isDir == 0) {							//���ļ� 
				char sub[4];
				strncpy(sub, ptrFcb->fileName + (strlen(ptrFcb->fileName) - 4), 4);
				if (strcmp(sub, ".txt") == 0) {
					PtrInode tempInode = (PtrInode)malloc(inodeSize);		//��¼��ǰInode
					searchInode(tempInode, ptrFcb->inum);
					printf("%d\t\t%s\n", tempInode->length, ptrFcb->fileName);
					CurTotalFileSize += tempInode->length;
					fileCount++;
				}
			}
		}
	}
	printf("\t\t\t\t\t%d���ļ�\n", fileCount);
	printf("\t\t\t\t\t%d��Ŀ¼\n", dirCount);


	free(ptrFcb);
	free(parentInode);
}

//�����ļ���Ŀ¼ 
void createFile(char *name, int isDir) {			//flag = 0�������ļ� flag = 1������Ŀ¼ 
	//int nowBlockNum;		//��¼Ҫ�õ�Block 
	//int nowInodeNum;  		//��¼Ҫ�õ�Inode 
	//PtrInode nowInode = (PtrInode)malloc(inodeSize);
	//PtrInode parentInode = (PtrInode)malloc(inodeSize);
	//PtrFcb fcb = (PtrFcb)malloc(fcbSize);


	////���ҿ��õ� block 
	//for (int i = 0; i < BLOCKNUM; i++) {
	//	if (superBlock.blockBitmap[i] == 0) {
	//		nowBlockNum = i;
	//		break;
	//	}
	//}

	////���ҿ��õ� inode 
	//for (int i = 0; i < INODENUM; i++) {
	//	if (superBlock.inodeBitmap[i] == 0) {
	//		nowInodeNum = i;
	//		break;
	//	}
	//}

	////��ʼ����Ŀ¼��inode 
	//nowInode->inum = nowInodeNum;
	//strcpy(nowInode->fileName, name);
	//if (isDir == 1)
	//	nowInode->isDir = 1;
	//else
	//	nowInode->isDir = 0;
	//nowInode->iparent = currentDir;
	//nowInode->length = 0;
	//nowInode->blockNum = nowBlockNum;

	////��ʼ��fcb  
	//strcpy(fcb->fileName, nowInode->fileName);
	//fcb->inum = nowInode->inum;
	//fcb->isDir = nowInode->isDir;

	////�Ѹ�Ŀ¼��inodeд���ļ�ϵͳ 
	//fseek(fp, superBlockSize + inodeSize * nowInodeNum, SEEK_SET);
	//fwrite(nowInode, inodeSize, 1, fp);
	//// cout<<"nowInode->inum:"<<nowInode->inum<<endl;

	// //���� superBlock �� bitmap  
	//superBlock.blockFree--;
	//superBlock.inodeFree--;
	//superBlock.blockBitmap[nowBlockNum] = 1;
	//superBlock.inodeBitmap[nowInodeNum] = 1;
	//rewind(fp);
	//fwrite(&superBlock, superBlockSize, 1, fp);		//дsuperBlock    

	////���¸�Ŀ¼��block 
	//fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//fread(parentInode, inodeSize, 1, fp);  			//parentInodeָ��Ŀ¼ 
	////д��ǰ�ļ���fcb 
	//fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE + parentInode->length * fcbSize, SEEK_SET);
	//fwrite(fcb, fcbSize, 1, fp);

	////���¸�Ŀ¼��inode 
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
		if (superBlock.blockBitmap[i] == 0)//�ҵ�һ������block��
		{
			index_block = i;
			superBlock.blockBitmap[i] = 1;//���Ϊ�Ѿ�ռ��
			break;
		}
	}
	for (int i = 0; i < INODENUM; i++)
	{
		if (superBlock.inodeBitmap[i] == 0)//�ҵ�һ������inode��
		{
			index_inode = i;
			superBlock.inodeBitmap[i] = 1;//���Ϊ�Ѿ�ռ��
			break;
		}
	}

	PtrInode nowinode = (PtrInode)malloc(sizeof(PtrInode));//��Ŀ¼�ڵ�
	PtrInode fatherinode = (PtrInode)malloc(sizeof(PtrInode));//�¸��׽ڵ�
	PtrFcb nowfcb = (PtrFcb)malloc(sizeof(PtrFcb));//���ļ��ڵ�

	//����nowinode�ڵ�
	nowinode->inum = index_inode;
	nowinode->blockNum = index_block;
	strcpy(nowinode->fileName, name);
	nowinode->iparent = currentDir;
	nowinode->isDir = isDir;
	nowinode->length = 0;

	//����fcb�ڵ�
	nowfcb->isDir = isDir;
	strcpy(nowfcb->fileName, nowinode->fileName);
	nowfcb->inum = nowinode->inum;

	//����superblock
	superBlock.blockFree--;
	superBlock.inodeFree--;
	
	//�ļ���д
	//д��nowinode
	fseek(fp, superBlockSize + inodeSize * index_inode, SEEK_SET);
	fwrite(nowinode, inodeSize, 1, fp);
	//д��superBlock
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//����fatherinode�ڵ�
	fread(fatherinode, sizeof(Inode),1, fp);
	//�����ӵ��ļ�д��
	fseek(fp, superBlockSize + INODENUM * inodeSize + fatherinode->blockNum * BLOCKSIZE + fatherinode->length * fcbSize, SEEK_SET);
	fwrite(nowfcb, fcbSize, 1, fp);
	//�޸ĸ�Ŀ¼
	fatherinode->length++;
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	fwrite(fatherinode, inodeSize, 1, fp);
	fflush(fp);
}

//����Ŀ¼ 
void md(char *name) {
	createFile(name, 1);
}




//�����ļ� 
void touch(char *name) {
	createFile(name, 0);
}

//д�ļ�
void write(char *name) {
	int fileInodeNum, cnt = 0;
	char c;
	PtrInode fileInode = (PtrInode)malloc(inodeSize);
	if ((fileInodeNum = findInodeNum(name, 1)) != -1) {
		printf("����һ��Ŀ¼������һ���ļ�\n");
		return;
	}

	fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1)
		printf("%s�ļ�������\n", name);
	else {
		//�ҵ��ļ��� inode 
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum * BLOCKSIZE, SEEK_SET);
		printf("�������ļ�����(�� # �Ž�β):\n");
		while ((c = getchar()) != '#') {
			fputc(c, fp);
			cnt++;
		}
		c = getchar();

		//����inode  
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fileInode->length = cnt;
		fseek(fp, -inodeSize, SEEK_CUR);
		fwrite(fileInode, inodeSize, 1, fp);
	}
	free(fileInode);
	fflush(fp);
}




//���ļ�
void more(char *name)
{
	char c;
	PtrInode fileInode = (PtrInode)malloc(inodeSize);
	int fileInodeNum = findInodeNum(name, 0);

	if (fileInodeNum == -1)
		printf("%s�ļ�������\n", name);
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

//�����ļ�
void copy(char *source, char* path)
{
	//�ҵ�Դ�ļ�
	int fileInodeNum = findInodeNum(source, 0);
	if (fileInodeNum == -1) {
		printf("�Ҳ���%s�ļ�\n", source);
		return;
	}
	else {
		//����Դ�ļ�			
		PtrInode sourceInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(sourceInode, inodeSize, 1, fp);
		//cout << sourceInode->inum<<endl; 
		fseek(fp, superBlockSize + INODENUM * inodeSize + sourceInode->blockNum * BLOCKSIZE, SEEK_SET); 	//fpָ��Դ�ļ������� 

		char content[50];
		char ch;
		int i;
		for (i = 0; i < sourceInode->length; i++) {
			ch = fgetc(fp);
			content[i] = ch;
		}
		content[i] = '\0';

		//����Ŀ���ļ� 
		cd(path);
		touch(source);			//��path·���´�����һ��sourceͬ���ļ� 
		PtrInode desInode = (PtrInode)malloc(inodeSize);		//desInode 		

		fseek(fp, superBlockSize + findInodeNum(source, 0) * inodeSize, SEEK_SET);
		fread(desInode, inodeSize, 1, fp);   						//desInodeָ��ͬ���ļ�		
		fseek(fp, superBlockSize + INODENUM * inodeSize + desInode->blockNum * BLOCKSIZE, SEEK_SET);  //desFileָ��ͬ���ļ���block

		for (i = 0; i < strlen(content); i++) {
			fputc(content[i], fp);
			fflush(fp);
		}

		//����inode  
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

//ɾ���ļ�
void del(char *name)
{
	if (findInodeNum(name, 0) == -1 && findInodeNum(name, 1) == -1)//δ�ҵ�ָ���ļ�����Ŀ¼
	{
		printf("δ�ҵ�ָ���ļ�����Ŀ¼\n");
	}
	else
	{	
		//��ȡ��Ŀ¼
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

		//ɾ����Ŀ¼������fcbĿ¼
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < parentInode->length; i++)
			fread(&fcb[i], fcbSize, 1, fp);  			//��ȡ��Ŀ¼������fcb 
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < BLOCKSIZE; i++)				//���blockȫ���  
			fputc(0, fp);

		//�ָ�ɾ�����˵��ļ�
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < parentInode->length; i++) {
			if ((strcmp(fcb[i].fileName, name)) != 0)  //����Ҫɾ�����ļ� 
				fwrite(&fcb[i], fcbSize, 1, fp);  		//д��block�� 
		}

		//���¸�Ŀ¼��inode��Ϣ 
		parentInode->length--;
		fseek(fp, superBlockSize + fileInode->iparent * inodeSize, SEEK_SET);
		fwrite(parentInode, inodeSize, 1, fp);

		//����superBlock
		superBlock.inodeBitmap[fileInodeNum] = 0;
		superBlock.blockBitmap[fileInode->blockNum] = 0;
		superBlock.blockFree++;
		superBlock.inodeFree++;
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);		//дsuperBlock
		free(fileInode);
		free(parentInode);
		fflush(fp);       
	}
	
}


//�������ļ�
void rename(char *originalName, char *newName)
{
	if ((findInodeNum(originalName, 0) == -1) && (findInodeNum(originalName, 1) == -1))
	{
		printf("δ���ҵ�ָ���ļ�����Ŀ¼\n");
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

		//��ȡ��Ŀ¼��fcb
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fseek(fp, superBlockSize + fileInode->iparent * inodeSize, SEEK_SET);
		fread(parentInode, inodeSize, 1, fp);

		//��ȡ��Ŀ¼������fcb
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE, SEEK_SET);
		for (i = 0; i < parentInode->length; i++) {
			fread(&fcb[i], fcbSize, 1, fp);  			
			if (fcb[i].inum == fileInodeNum)
				break;
		}

		//��ԭ�ļ�������
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE + i * fcbSize, SEEK_SET); 	//ָ���Ǹ�fcb
		fread(ptrFcb, fcbSize, 1, fp);
		memset(ptrFcb->fileName, '\0', sizeof(ptrFcb->fileName));
		strcpy(ptrFcb->fileName, newName);
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum * BLOCKSIZE + i * fcbSize, SEEK_SET); 	//ָ���Ǹ�fcb
		fwrite(ptrFcb, fcbSize, 1, fp);
		fflush(fp);
	}
}


//ɾ���ļ���
void rd(char *name)
{
	int fileInodeNum = findInodeNum(name, 0);
	int dirInodeNum = findInodeNum(name, 1);
	if (fileInodeNum == -1 && dirInodeNum == -1) {
		printf("ɾ��ʧ�ܣ���������ȷ���ļ�����\n");
		return;
	}
	if (fileInodeNum != -1 && dirInodeNum == -1) { 		//���ļ� 	
		//ֱ��ɾ�� 
		del(name);
		return;
	}
	else if (dirInodeNum != -1 && fileInodeNum == -1) {			//��Ŀ¼	 
		PtrInode ptrInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + dirInodeNum * inodeSize, SEEK_SET);
		fread(ptrInode, inodeSize, 1, fp);  		//ptrInodeָ��ǰĿ¼			
		if (ptrInode->length != 0) {				//��Ŀ¼�»����ļ�
			cd(ptrInode->fileName);
			PtrFcb fcb = (PtrFcb)malloc(fcbSize);
			fseek(fp, superBlockSize + INODENUM * inodeSize + ptrInode->blockNum * BLOCKSIZE, SEEK_SET);

			for (int i = 0; i < ptrInode->length; i++) {
				fseek(fp, superBlockSize + INODENUM * inodeSize + ptrInode->blockNum * BLOCKSIZE, SEEK_SET);
				fread(fcb, fcbSize, 1, fp);  			//��ȡĿ¼������fcb 
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


//export �����ļ�������
void export_(char *name, char *path) {
	FILE *ft;
	path = strcat(path, name);
	if ((ft = fopen(path, "w")) != NULL) {
		int fileInodeNum = findInodeNum(name, 0);
		if (fileInodeNum == -1) {
			printf("%s�ļ������ڣ�", name);
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
		printf("���ļ�ʧ�ܣ�\n");
	}
	fclose(ft);
}


//inport �������ļ�����
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

		touch(filename);			//�ڵ�ǰ·���´���ͬ���ļ� 

		PtrInode fileInode = (PtrInode)malloc(inodeSize);
		fseek(fp, superBlockSize + findInodeNum(filename, 0) * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);   						//desInodeָ��ͬ���ļ�		
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum * BLOCKSIZE, SEEK_SET);  //ָ��ͬ���ļ���block

		int cntNum = 0;
		char c;
		while ((c = fgetc(ftemp)) != EOF) {			//д��block��ȥ 
			fputc(c, fp);
			fflush(fp);
			cntNum++;
		}

		//����inode  
		fseek(fp, superBlockSize + findInodeNum(filename, 0) * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		fileInode->length = cntNum;
		fseek(fp, -inodeSize, SEEK_CUR);
		fwrite(fileInode, inodeSize, 1, fp);
		fflush(fp);
	}
	else {
		printf("���ļ�ʧ�ܣ�\n");
	}
	fclose(ftemp);

}


//�ƶ��ļ�
void move(char *source, char *des) {
	int sourceInodeNum = findInodeNum(source, 0);
	int desInodeNum = findInodeNum(des, 1);
	if (sourceInodeNum == -1) {
		printf("%s�ļ������ڣ�", source);
		return;
	}
	if (desInodeNum == -1) {
		printf("·������");
		return;
	}
	copy(source, des);
	del(source);
}


//��ʾ�˵�
void help() {
	cout << "===========================�ļ�ϵͳ=================================\n";
	cout << "   cd [Path]                        Ŀ¼�л�\n";
	cout << "   dir                              ��ʾĿ¼����\n";
	cout << "   copy [FileName] [DirPath]        �����ļ�\n";
	cout << "   del [FileName]                   ɾ���ļ�\n";
	cout << "   md [DirName]                     ����Ŀ¼\n";
	cout << "   more [FileName]                  �鿴�ļ�\n";
	cout << "   rd [DirName]                     ɾ��Ŀ¼\n";
	cout << "   move [FileName] [DirName]        ת���ļ�\n";
	cout << "   help                             ��ʾ������Ϣ\n";
	cout << "   rename [OldName] [NewName]       �ļ���Ŀ¼������\n";
	cout << "   touch [FileName]                 �����ļ�\n";
	cout << "   write [FileName]                 д�ļ�\n";
	cout << "   exit                             �˳�\n";
	cout << "   cls                              ����\n";
	cout << "   import [Path]                    �ӱ��ش��̸����ļ�����ǰĿ¼\n";
	cout << "   export [FileName] [Path]         ����ǰĿ¼�µ��ļ����������ش���\n";
	cout << "====================================================================\n";
}


//const char *syscmd[] = {"cd", "dir", "copy", "del", "md", "more", "rd", "move", "help", "time", "ver", "rename", "touch", "write", "exit", "cls", "import", "export"};
void command() {
	char cmd[30];
	do
	{
		WaitForSingleObject(Mutex, INFINITE);//ֻ����һ���̹߳���
		//FILE* f = fopen(FILENAME, "rb+");
		setPath();
		cin.getline(cmd,1024);
		for (int i = 0; i < 5; i++)
		{
				argv[i] = (char *)malloc(1024);//Ϊָ����ڴ�
				argv[i][0] = '\0';//����ʼֵ
		}
		switch (splitCommand(cmd)) {
		case 0:  		//cd
			cd(argv[1]);			//cd dira��argv[1]��dira 
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
		case 6:  		//rd��ɾ���ļ��� 
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
			ReleaseMutex(Mutex);//�ͷ��߳�
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
			printf("�������\n");
			ReleaseMutex(Mutex);//�ͷ��߳�
			break;
		}
	} while (1);
}




//�����ļ�ϵͳ 
void createFileSystem() {
	PtrInode ptrInode;
	if ((fp = fopen(FILENAME, "wb+")) == NULL) {  			//wb+���Զ����Ʒ�ʽ���ж�д
		printf("���ļ�ʧ�ܣ�\n", FILENAME);
		exit(1);
	}

	//��ʼ�� bitmap  
	for (int i = 0; i < BLOCKNUM; i++)
		superBlock.blockBitmap[i] = 0;				//δʹ��  

	for (int i = 0; i < INODENUM; i++)
		superBlock.inodeBitmap[i] = 0;  				//δʹ�� 


	for (int i = 0; i < (superBlockSize + inodeSize * INODENUM + BLOCKSIZE * BLOCKNUM); i++)
		fputc(0, fp);  //���ַ�0д���ļ�ָ��fp��ָ����ļ��ĵ�ǰдָ���λ�� 
	rewind(fp); 			//ʹ�ļ�λ��ָ�����¶�λ��fp�ļ��Ŀ�ʼλ�á� 

	//��ʼ��superBlock  
	superBlock.blockNum = BLOCKNUM;
	superBlock.blockSize = BLOCKSIZE;
	superBlock.inodeNum = INODENUM;
	superBlock.blockFree = BLOCKNUM - 1;
	superBlock.inodeFree = INODENUM - 1;

	//0��inode�Ǹ�Ŀ¼ 
	//������Ŀ¼  
	ptrInode = (PtrInode)malloc(inodeSize);
	ptrInode->inum = 0;
	strcpy(ptrInode->fileName, "/");  //���ڵ� 
	ptrInode->isDir = 1;  			   //Ŀ¼ 
	ptrInode->iparent = 0;
	ptrInode->length = 0;
	ptrInode->blockNum = 0;

	//��Ŀ¼���� 
	superBlock.inodeBitmap[0] = 1;
	superBlock.blockBitmap[0] = 1;

	//дsuperBlock 
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);		//дsuperBlock      

	//д��Ŀ¼
	fseek(fp, superBlockSize, SEEK_SET);
	fwrite(ptrInode, inodeSize, 1, fp);

	//��ǰĿ¼��ʼ��Ϊ��Ŀ¼ 
	currentDir = 0;
	fflush(fp);
}






//���ļ�ϵͳ 
void openFileSystem()
/*���FILENAME�ɶ��������֮ǰ������Ϣ������ȡ��Ӧ����    ������ɶ����򴴽��ļ�ϵͳ */
{
	if ((fp = fopen(FILENAME, "rb+")) != NULL) {			//�����ļ�ϵͳ 
		if ((fp = fopen(FILENAME, "rb+")) == NULL) {  	//rb+ �ɶ���д��ʽ����һ���������ļ��������ڻᱨ�� 
			printf("���ļ�ʧ�ܣ�\n", FILENAME);
			exit(1);
		}
		rewind(fp);  		//ʹ�ļ�λ��ָ�����¶�λ��fp�ļ��Ŀ�ʼλ�á�

		//�������� 
		fread(&superBlock, superBlockSize, 1, fp);
		//��ʼ����ǰĿ¼ 
		currentDir = 0;			//��ǰĿ¼Ϊ��Ŀ¼  	
	}
	else											//�������ļ�ϵͳ 
		createFileSystem();
}






int main() {
	//signal(SIGINT, stopHandle);
	//	login();   
	Mutex = CreateMutex(NULL, FALSE, NULL);//�����߳�
	openFileSystem();  			//���ļ�ϵͳ 
	help();
	command();
	return 0;
}
































