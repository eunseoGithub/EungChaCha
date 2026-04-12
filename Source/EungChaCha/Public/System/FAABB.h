#pragma once

// AABB = Axis-Aligned Bounding Box
// 축 정렬 박스 -> 회전 없음, 직사각형 범위만 표현
struct FAABB
{
	FVector2D Min; // 좌하단 (x최소, y최소)
	FVector2D Max; // 우상단 (x최대, y최대)
	
	//중심 + 반크기로 생성
	static FAABB FromCenterExtent(FVector2D Center, FVector2D HalfExtent)
	{
		return {Center - HalfExtent, Center + HalfExtent};
	}
	
	// 중심점 반환
	FVector2D GetCenter() const
	{
		return (Min + Max) * 0.5f;
	}
	
	// 반크기 반환
	FVector2D GetHalfExtent() const
	{
		return (Max - Min) * 0.5f;
	}
	
	// 점이 박스 안에 있는지
	bool Contains(FVector2D Point) const
	{
		return Point.X >= Min.X && Point.X <= Max.X
		&& Point.Y >= Min.Y && Point.Y <= Max.Y;
	}
	
	// 두 AABB가 겹치는지
	bool Intersects(const FAABB& Other) const
	{
		if (Max.X < Other.Min.X || Min.X > Other.Max.X) return false;
		if (Max.Y < Other.Min.Y || Min.Y > Other.Max.Y) return false;
		return true;
	}
	
	// 원(돌멩이)과 겹치는지
	bool IntersectsCircle(FVector2D Center, float Radius) const
	{
		// 박스에서 원 중심까지 가장 가까운 점 계산
		float CloseX = FMath::Clamp(Center.X, Min.X, Max.X);
		float CloseY = FMath::Clamp(Center.Y, Min.Y, Max.Y);
		
		float DX = Center.X - CloseX;
		float DY = Center.Y - CloseY;
		
		return (DX * DX + DY * DY) <= (Radius * Radius);
	}
};
