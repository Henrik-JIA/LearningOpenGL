#pragma once
#ifndef __Tool_h__
#define __Tool_h__

#include <time.h>
#include <stdlib.h>

// 初始化CPU随机数生成器
void CPURandomInit() {
	srand(time(NULL));
}

// 获取CPU随机数
float GetCPURandom() {
	return (float)rand() / (RAND_MAX + 1.0);
}


#endif

