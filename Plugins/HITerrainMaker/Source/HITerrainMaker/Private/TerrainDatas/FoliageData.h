// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/*
 * 植被信息
 */

struct FFoliageData
{
public:
	// 植被位置
	FVector Location;

	// 植被旋转
	FRotator Rotation;

	// 植被类型
	uint8 Type;
};