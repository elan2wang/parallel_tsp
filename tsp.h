
#ifndef _TSP_H
#define _TSP_H

#include<utility>
#include<stdio.h>
#include<stdlib.h>
//#include<cv.h>

using namespace std;

#define MAXCITIES 100000	// 城市数量
#define CARDINALITY 10		// 种群基数
#define CROSSOVER_RATE 0.8	// 交配率
#define MUTATE_RATE 0.01	// 突变率

#define MAXPROCS 10			// 最大并发数
#define MAXN 10             // 从进程提交给主进程的最优个体数最大值

typedef pair<double, double> City;	//城市坐标表示

typedef struct{
	City	path[MAXCITIES];	// 城市序列
	double	fitness;			// 适应度 （用路径长度表示）
	double	rfitness;			// 适应率
	double	ifitness;			// 轮盘区间的上边界
} GeneType;

extern	GeneType	population[CARDINALITY+1];		//种群
extern	GeneType	newpopulation[CARDINALITY+1];	//选择后的新种群
extern	GeneType	selectedGenes[MAXN];			//选取的N个最优路径

extern	int		numOfCities;			// 城市数目
extern	int		optimal;				// 最短路经
extern	int		myid;					// 进程编号
extern	char	processor_name[100];	// 处理器名称
extern	double	mytime;					// 总时间
extern	double	length;					// 最优路径长度

/* implemented in graph.cpp */
//CvSize normalize(City* path, int num);
//void   draw(City* path, int num);

/* implemented in tsp.cpp */
void   initialize(const char* file);
void   swap(City* a, City* b);
double dist(City c1, City c2);
void   evaluate();
void   select_best();
int    compare(const void* elem1, const void* elem2);
void   assign(GeneType* dest, GeneType* src);
void   select_N_best(int n);
void   select();
int    prefer();
void   crossover();
void   mutate();

#endif
