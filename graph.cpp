#include<cv.h>
#include<highgui.h>
#include<utility>

#include"tsp.h"

CvPoint points[MAXCITIES];	//城市点集
IplImage* pImg;				//声明图像指针

/* 标准化图片大小 */
CvSize normalize(City* path, int num)
{
	double minX = 1000000.0;
	double minY = 1000000.0;
	double maxX = 0.0;
	double maxY = 0.0;

	for(int i=0; i<num; i++)
	{
		if(path[i].first < minX)
			minX = path[i].first;
		if(path[i].second < minY)
			minY = path[i].second;
		if(path[i].first > maxX)
			maxX = path[i].first;
		if(path[i].second > maxY)
			maxY = path[i].second;
	}	
	
	for(int i=0; i<num; i++)
	{
		path[i].first = (path[i].first - minX)/2.0 + 20.0;
		path[i].second = (path[i].second - minY)/2.0 + 20.0;
	}

	return cvSize((maxX-minX)/2.0 + 40, (maxY-minY)/2.0 + 40);
}

void draw(City* path, int num)
{
	// 标准化
	CvSize size = normalize(path, num);

	CvScalar line_color = CV_RGB(100,150,200);
	
	pImg = cvCreateImage(size, IPL_DEPTH_8U, 3); 

	for(int i=0; i<num; i++)
	{
		CvPoint point = cvPoint(path[i].first, path[i].second);
		points[i] = point;
		cvLine(pImg, points[i],points[i],CV_RGB(255,0,0),3);
	}
	for(int i=1; i<num; i++)
		cvLine(pImg, points[i-1],points[i],line_color);
	cvLine(pImg, points[num-1], points[0], line_color);
	
	// 设置名称字符串
	char name[1000];
	sprintf(name, "NumofCities[%d],Time[%.2lf],Length[%.2lf]", numOfCities, mytime, length);

	cvNamedWindow(name,1);		//创建窗口
	cvShowImage(name,pImg);		//显示图像
	cvWaitKey(0);				//等待按键
	cvDestroyWindow(name);		//销毁窗口
	cvReleaseImage(&pImg);		//释放图像
}
