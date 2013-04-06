#include<stdio.h>
#include<stdlib.h>
#include"tsp.h"
#include"mpi.h"

enum {PUT_BETTER_TAG, SEND_BETTER_TAG, DONE_TAG };

MPI_Datatype MPI_CITY;          // 定义City的MPI类型
MPI_Datatype MPI_GENETYPE;      // 定义GeneType的MPI类型

double	length = 0;				// 最短路径长度
double	mytime = 0;				// 最终实验时间
double  start;                  // 开始时间
double  finish;                 // 结束时间

int		margin = 0;				// 与最优路经的距离差
int		myid;					// 进程编号
int		numprocs;				// 进程数
char	processor_name[100];	// 处理器名称	
int		namelen;				// 处理器名称长度

int		n = 2;					// 每次提交最好的2个路经
int		generations = 10000;	// 一个周期的繁殖代数
GeneType	revGenes[MAXN*MAXPROCS];	// 保存从进程提交的路径值

/* master 进程  */
void coordinator()
{
	printf("\tProcessor %d at %s begin work..\n", myid, processor_name);

	MPI_Status status[MAXPROCS];
	MPI_Request handle[MAXPROCS];
	int recv_flag = 0;

	while(1)
	{
		// 非阻塞接收所有从进程提交的消息
		for(int i=1; i < numprocs; i++)
		{
			MPI_Irecv(revGenes+(i-1)*n, n, MPI_GENETYPE, i, MPI_ANY_TAG, MPI_COMM_WORLD, handle+(i-1));
		}
		// 循环等待非阻塞接收完成
		while(!recv_flag)
		{
			// 测试所有接收是否完成
			MPI_Testall(numprocs-1, handle, &recv_flag, status);
			// 判断是否有收到DONE_TAG消息
			for(int i=0; i< numprocs-1; i++)
			{
				if(status[i].MPI_TAG == DONE_TAG)
				{
					// 将最优个体保存在population[CARDINALITY]中
					assign(&population[CARDINALITY],&revGenes[(status[i].MPI_SOURCE-1)*n]);
					// 发送终止消息给其它从进程
					for(int j=1; j < numprocs; j++)
					{
						if(j != status[i].MPI_SOURCE)
							MPI_Send(NULL, 0, MPI_INT, j, DONE_TAG, MPI_COMM_WORLD);
					}
					printf("\tProcessor %d at %s exit\n", myid, processor_name);
					// 设置时间和长度
					mytime = MPI_Wtime() - start;
					length = population[CARDINALITY].fitness;
					// 输出结果
					printf("\n\t***************** Results *********************\n");
					printf("\t**** Time  ：%lf \n", mytime);
					printf("\t**** Length: %lf \n", length);
					printf("\t***********************************************\n");
					// 绘制图形
					//draw(population[CARDINALITY].path, numOfCities);
					return;
				}
			}
		}
		// 重置标志
		recv_flag = 0;
		// 对接收到的优良个体排序，取最优的n个发送给所有从进程
		qsort(revGenes, (numprocs-1)*n, sizeof(GeneType), compare);
		for(int i=0; i < n; i++)assign(&selectedGenes[i],&revGenes[i]);
		for(int i=1; i < numprocs; i++)
			MPI_Send(selectedGenes, n, MPI_GENETYPE, i, SEND_BETTER_TAG, MPI_COMM_WORLD);
		
		printf("\tcurrrent shortest path is %lf\n", revGenes[0].fitness);
	}
}
/* slave 进程 */
void worker()
{
	printf("\tProcessor %d at %s begin work..\n", myid, processor_name);
	
	MPI_Status status;
	MPI_Request handle;
	
	int recv_flag = 0;	
	int count = 0;
	int upload = 0;
	
	// 非阻塞接收主进程消息
	MPI_Irecv(selectedGenes, n, MPI_GENETYPE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &handle);
	
	while(1)
	{	
		// 独立繁衍count代
		count = generations;
		while(count--)
		{
			select();
			crossover();
			mutate();
			evaluate();
			prefer();
			// 若满足终止条件，则向主进程发送最优路径，并结束进程
			if(population[CARDINALITY].fitness <= optimal+margin)
			{
				printf("\tProcessor %d at %s Terminated\n", myid, processor_name);
				MPI_Send(&population[CARDINALITY], 1, MPI_GENETYPE, 0, DONE_TAG, MPI_COMM_WORLD);
				printf("\tProcessor %d at %s exit\n", myid, processor_name);
				return;
			}
			// 探测是否收到主进程的消息
			MPI_Test(&handle, &recv_flag, &status);
			// 若收到主进程的消息
			if(recv_flag)
			{
				printf("\tProcessor %d at %s recv %d\n", myid, processor_name, status.MPI_TAG);

				// 状态重置
				recv_flag = 0;
				// 若接收到DONE_TAG则结束进程
				if(status.MPI_TAG == DONE_TAG)
				{
					printf("\tProcessor %d at %s exit\n", myid, processor_name);
					return;
				}
				// 否则，将接收到的优良个体替换种群中最差的个体
				qsort(population, CARDINALITY, sizeof(GeneType), compare);
				for(int i=1; i <= n; i++)
					assign(&population[CARDINALITY-i], &selectedGenes[i-1]);
				if(selectedGenes[0].fitness < population[CARDINALITY].fitness)
					assign(&population[CARDINALITY], &selectedGenes[0]);

				// 非阻塞接收主进程消息
				MPI_Irecv(selectedGenes, n, MPI_GENETYPE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &handle);
			}
		}
		// 繁衍count代后，若没有终止则向主进程发送最优个体
		select_N_best(n);
		MPI_Send(selectedGenes, n, MPI_GENETYPE, 0, PUT_BETTER_TAG, MPI_COMM_WORLD);	
		printf("\tProcessor %d at %s upload %d\n", myid, processor_name, upload++);
	}
}

/* 初始化  */
void init()
{
	if(myid == 0)
	{	
		/* 初始化参数和种群 */
		initialize("./data/data.in");
		printf("\n\n\t*********** Setting Parameters *********************\n");
		printf("\t** 1. Number of  Cities: %d\n", numOfCities);
		printf("\t** 2. The optimal value: %d\n", optimal);
		printf("\t** 3. Gap   to  optimal: ");
		scanf("%d", &margin);
		printf("\t************ Starting Simulation ********************\n\n");
		
		evaluate();
		select_best();
		start = MPI_Wtime();
	}

	/* 广播城市个数 */
	MPI_Bcast(&numOfCities, 1, MPI_INT, 0, MPI_COMM_WORLD);
	/* 广播最短路径 */
	MPI_Bcast(&optimal, 1, MPI_INT, 0, MPI_COMM_WORLD);
	/* 广播与最优路径的最大差值 */
	MPI_Bcast(&margin, 1, MPI_INT, 0, MPI_COMM_WORLD);
	/* 广播初始种群 */
	MPI_Bcast(population, CARDINALITY+1, MPI_GENETYPE, 0, MPI_COMM_WORLD);
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Get_processor_name(processor_name,&namelen);
	
	/* 自定义MPI数据类型 */
	MPI_Type_contiguous(2, MPI_DOUBLE, &MPI_CITY);
	MPI_Type_commit(&MPI_CITY);

	MPI_Datatype types[4] = {MPI_CITY, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE};
	int lengths[4] = {MAXCITIES, 1, 1, 1};

	MPI_Aint disp[4];
	int base;
	MPI_Address(population, disp);
	MPI_Address(&population[0].fitness, disp+1);
	MPI_Address(&population[0].rfitness, disp+2);
	MPI_Address(&population[0].ifitness, disp+3);
	base = disp[0];
	for(int i = 0; i < 4; i++) disp[i] -= base;
	
	MPI_Type_struct(4, lengths, disp, types, &MPI_GENETYPE);
	MPI_Type_commit(&MPI_GENETYPE);
	/* 自定义结构结束  */

	init();

	if(myid == 0)
		coordinator();
	else
		worker();

	MPI_Finalize();
	return 0;
}
