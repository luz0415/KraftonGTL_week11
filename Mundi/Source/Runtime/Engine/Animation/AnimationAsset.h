#pragma once
#include "Object.h"

/**
 * UAnimationAsset
 * 모든 애니메이션 에셋의 베이스 클래스
 */
class UAnimationAsset : public UObject
{
public:
	DECLARE_CLASS(UAnimationAsset, UObject)

	UAnimationAsset();
	virtual ~UAnimationAsset() override;
};
