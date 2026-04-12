#pragma once

#include "AssetTypeCategories.h"
#include "FQuadTreeNode.h"

struct FQuadTree
{
	TArray<FQuadTreeNode> Nodes;
	
	//초기화 : 루트 노드 생성
	void Init(FAABB WorldBounds)
	{
		Nodes.Empty();
		FQuadTreeNode Root;
		Root.Boundary = WorldBounds;
		Root.Depth = 0;
		Root.bSubdivided = false;
		Nodes.Add(Root);
	}
	
	//장애물 삽입
	void Insert(int32 NodeIdx, const FAABB& Obstacle)
	{
		FQuadTreeNode& Node = Nodes[NodeIdx];
		
		//이 노드 범위 밖이면 무시
		if (!Node.Boundary.Intersects(Obstacle)) return;
		
		//아직 분할 안 됐고 여유 있으면 여기에 저장
		if (!Node.bSubdivided && Node.Obstacles.Num() < FQuadTreeNode::MAX_CAPACITY)
		{
			Node.Obstacles.Add(Obstacle);
			return;
		}
		
		//꽉 찼으면 4분활
		if (!Node.bSubdivided && Node.Depth < FQuadTreeNode::MAX_DEPTH)
		{
			Subdivide(NodeIdx);
		}
		
		//자식 노드들에 재귀 삽입
		if (Node.bSubdivided)
		{
			Insert(Node.ChildNW, Obstacle);
			Insert(Node.ChildNE, Obstacle);
			Insert(Node.ChildSW, Obstacle);
			Insert(Node.ChildSE, Obstacle);
		}
		else
		{
			//MAX_DEPTH 도달 -> 그냥 여기에 저장
			Node.Obstacles.Add(Obstacle);
		}
	}
	
	//특정 범위와 겹치는 장애물 수집
	void Query(int32 NodeIds, const FAABB& Range,
		TArray<FAABB>& OutObstacles) const
	{
		const FQuadTreeNode& Node = Nodes[NodeIds];
		
		//범위 밖이면 스킵
		if (!Node.Boundary.Intersects(Range)) return;
		
		//이 노드의 장애물 추가
		for (const FAABB& Obs : Node.Obstacles)
		{
			if (Obs.Intersects(Range))
				OutObstacles.Add(Obs);
		}
		
		//자식 노드 재귀 탐색
		if (Node.bSubdivided)
		{
			Query(Node.ChildNW, Range,OutObstacles);
			Query(Node.ChildNE, Range,OutObstacles);
			Query(Node.ChildSW, Range,OutObstacles);
			Query(Node.ChildSE, Range,OutObstacles);
		}
	}
private:
	//노드를 4등분
	void Subdivide(int32 NodeIdx)
	{
		FQuadTreeNode& Node = Nodes[NodeIdx];
		FVector2D Center = Node.Boundary.GetCenter();
		FVector2D HalfExt = Node.Boundary.GetHalfExtent();
		FVector2D QuarterExt = HalfExt * 0.5f;
		int32 Depth = Node.Depth + 1;
		
		auto MakeChild = [&](FVector2D ChildCenter) -> int32
		{
			FQuadTreeNode Child;
			Child.Boundary = FAABB::FromCenterExtent(ChildCenter, QuarterExt);
			Child.Depth = Depth;
			Child.bSubdivided = false;
			Nodes.Add(Child);
			return Nodes.Num()-1;
		};
		
		// 참조 무효화 방지: 인덱스 먼저 저장
		int32 NW = MakeChild(FVector2D(Center.X - QuarterExt.X, Center.Y + QuarterExt.Y));
		int32 NE = MakeChild(FVector2D(Center.X + QuarterExt.X, Center.Y + QuarterExt.Y));
		int32 SW = MakeChild(FVector2D(Center.X - QuarterExt.X, Center.Y - QuarterExt.Y));
		int32 SE = MakeChild(FVector2D(Center.X + QuarterExt.X, Center.Y - QuarterExt.Y));
		
		// Add 후 재참조 (Nodes 배열 재할당 가능성 때문에 인덱스로 접근)
		Nodes[NodeIdx].ChildNW   = NW;
		Nodes[NodeIdx].ChildNE   = NE;
		Nodes[NodeIdx].ChildSW   = SW;
		Nodes[NodeIdx].ChildSE   = SE;
		Nodes[NodeIdx].bSubdivided = true;
	}
	
};
