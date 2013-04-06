#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<fstream>
#include<unistd.h>
#include"tsp.h"

GeneType population[CARDINALITY+1];     // 种群
GeneType newpopulation[CARDINALITY+1];  // 选择后的新种群
GeneType selectedGenes[MAXN];		// N个最好的路经

int numOfCities =0;        	// 城市数目
int optimal =0;            	// 最短路经
int rand_seed = 0;			// 随机数种子

// 交换两个城市的顺序
void swap(City* a, City* b)
{
	City temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

// 计算两个城市间的距离
double dist(City c1, City c2)
{
	return sqrt((c1.first-c2.first)*(c1.first-c2.first)+(c1.second-c2.second)*(c1.second-c2.second));
}

// 评估种群中个体的适应度
void evaluate()
{
	int next;
	
	for(int i=0; i < CARDINALITY; i++)
	{
		population[i].fitness = 0;
		for(int j=0; j < numOfCities; j++)
		{
			if (j == numOfCities-1) next = 0;
			else next = j+1;
			population[i].fitness += dist(population[i].path[j], population[i].path[next]);		
		}
	}
}

/* 给GeneType赋值 */
void assign(GeneType* dest, GeneType* src)
{
	for(int i=0; i < numOfCities; i++)
	{
		dest->path[i] = src->path[i];
	}
	dest->fitness = src->fitness;
	dest->ifitness = src->ifitness;
	dest->rfitness = src->rfitness;
}

//初始化种群
void initialize(const char* file)
{
	double valueX,valueY;

	// 从文本中加载城市坐标数据，并保存在种群的第一个个体中
	ifstream input(file);
	if(input != NULL)
	{
		input >> optimal;
		while(input >> valueX >> valueY)
		{
			City city (valueX, valueY);
			population[0].path[numOfCities++] = city;
		}
	}
	input.close();

	// 给种群的剩余个体赋值
	for(int i=1; i < CARDINALITY; i++)
	{
		for(int j=0; j < numOfCities; j++)
		{
			population[i].path[j] = population[0].path[j];
		}
	}

	// 随机化种群中所有个体的城市序列
	for(int i=0; i < CARDINALITY; i++)
	{
		for(int j=0; j< numOfCities; j++)
		{
			swap(&population[i].path[j],&population[i].path[rand()%numOfCities]);
		}
	}
}

// 选择最优个体并保存在population[CARDINALITY]中
void select_best()
{
	double best = 100000000000;
	GeneType bestGene;
	for(int i =0; i < CARDINALITY; i++)
	{
		if(population[i].fitness < best)
		{
			best = population[i].fitness;
			bestGene = population[i];
		}
	}
	assign(&population[CARDINALITY],&bestGene);
}

/* 增序排列 */
int compare(const void* elem1, const void* elem2)
{
	return (*(GeneType*)elem1).fitness > (*(GeneType*)elem2).fitness ? 1 : -1; 
}

/* 选取N个最优路径 */
void select_N_best(int n)
{
	qsort(population, CARDINALITY, sizeof(GeneType), compare);
	for(int i=0; i<n;i++)assign(&selectedGenes[i], &population[i]);
}

// 轮盘选择，选择淘汰适应度小的
void select()
{
	double sum_fitness = 0.0;
	double p = 0.0;
	double x[numOfCities];

	/* 计算适应度总值 */
	for(int i=0; i < CARDINALITY; i++) sum_fitness += population[i].fitness;
	/* 计算新的适应度 */
	for(int i=0; i < CARDINALITY; i++) x[i] = sum_fitness - population[i].fitness;
	/* 计算新的适应度总值 */
	for(int i=0; i < CARDINALITY; i++) sum_fitness += x[i];
	/* 计算适应率  */
	for(int i=0; i < CARDINALITY; i++) population[i].rfitness = x[i]/sum_fitness;
	/* 计算轮盘对应区间 */
	population[0].ifitness = population[0].rfitness;
	for(int i=1; i < CARDINALITY; i++) population[i].ifitness = population[i-1].ifitness + population[i].rfitness;

//	for(int i=0; i< CARDINALITY; i++) printf("population[%d].ifitness = %lf\n", i, population[i].ifitness);
	/* 通过轮盘选择种群 */
	srand(getpid()+rand_seed++);
	for(int i = 0; i < CARDINALITY; i++)
	{
		p = rand()/(double)(RAND_MAX);
//		printf("p is %lf\n",p);

		if(p < population[0].ifitness)
			assign(&newpopulation[i], &population[0]);
		else if(p >= population[CARDINALITY-1].ifitness)
			assign(&newpopulation[i], &population[CARDINALITY-1]);
		else
			for(int j = 1; j < CARDINALITY; j++)
				if(p >=population[j-1].ifitness && p < population[j].ifitness)
					assign(&newpopulation[i],&population[j]);
	}

	/* 将新种群替换旧种群 */
	for(int i=0; i < CARDINALITY; i++)
		assign(&population[i],&newpopulation[i]);

}

/* 择优 */ 
/* 1.用父辈的最优个体替换当代的最差个体 */
/* 2.若当代最优个体优于保存的最优个体，则将其替换 */
int prefer()
{
	double best = population[0].fitness;
	double worst = population[0].fitness;
	int best_index = 0;
	int worst_index = 0;

	for(int i=1; i < CARDINALITY; i++)
	{
		if(population[i].fitness < best)
		{
			best = population[i].fitness;
			best_index = i;
		}
		if(population[i].fitness > worst)
		{
			worst = population[i].fitness;
			worst_index = i;
		}
	}
	
	assign(&population[worst_index], &population[CARDINALITY]);
	
	if (best < population[CARDINALITY].fitness)
	{
		assign(&population[CARDINALITY], &population[best_index]);
		return 1;	// 最优值发生改变
	}
	return 0;		// 最优值未改变
}

// 交配，实质为将一段路径逆序
void crossover()
{
	double x = 0.0;
	int begin, end, tmp;

	for(int i =0; i < CARDINALITY; i++)
	{
		x = rand()/(double)(RAND_MAX);
		if(x < CROSSOVER_RATE)
		{
			begin = rand()%numOfCities;
			end = rand()%numOfCities;
			while(end == begin) end = rand()%numOfCities;
			if(end < begin)
			{
				tmp = begin;
				begin = end;
				end = tmp;
			}

			int flag = end;
			for(int j = begin; j <= (begin+end)/2; j++)
			{
				swap(&population[i].path[j], &population[i].path[flag]);
				flag = flag - 1;
			}
		}
	}
}

// 突变, 随机交换某个体中两个城市的位置
void mutate()
{
	double x = 0.0;
	int pos1,pos2;

	for(int i =0; i < CARDINALITY; i++)
	{
		x = rand()/(double)(RAND_MAX);
		if(x < MUTATE_RATE)
		{
			pos1 = rand()%numOfCities;
			pos2 = rand()%numOfCities;
			while(pos2 == pos1)	pos2 = rand()%numOfCities;
			
			swap(&population[i].path[pos1], &population[i].path[pos2]);
		}
	}
}


