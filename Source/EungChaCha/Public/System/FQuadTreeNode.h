#pragma once

#include "FAABB.h"

struct FQuadTreeNode
{
	FAABB Boundary;//이 노드가 커버하는 영역
	int32 Depth;//현재 깊이
	bool bSubdivided;//4분할 됐는지
	
	TArray<FAABB> Obstacles;//이 노드에 저장된 장애물들
	
	//자식 노드 인덱스 (별도 TArray로 관리)
	int32 ChildNW = -1;//북서 (좌상단)
	int32 ChildNE = -1;//불동 (우상단)
	int32 ChildSW = -1;//남서 (좌하단)
	int32 ChildSE = -1;//남동 (우하단)
	
	static const int32 MAX_CAPACITY = 4;//노드 당 최대 장애물 수
	static const int32 MAX_DEPTH = 6;//최대 분할 깊이
};
